#include "backend/HttpServer.h"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(fd) ::close(fd)
#endif

namespace freebuff::backend {

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    std::string reason;
    switch (statusCode) {
        case 200: reason = "OK"; break;
        case 201: reason = "Created"; break;
        case 204: reason = "No Content"; break;
        case 301: reason = "Moved Permanently"; break;
        case 304: reason = "Not Modified"; break;
        case 400: reason = "Bad Request"; break;
        case 401: reason = "Unauthorized"; break;
        case 403: reason = "Forbidden"; break;
        case 404: reason = "Not Found"; break;
        case 405: reason = "Method Not Allowed"; break;
        case 409: reason = "Conflict"; break;
        case 422: reason = "Unprocessable Entity"; break;
        case 429: reason = "Too Many Requests"; break;
        case 500: reason = "Internal Server Error"; break;
        default: reason = "Unknown"; break;
    }
    oss << "HTTP/1.1 " << statusCode << " " << reason << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "Access-Control-Allow-Origin: *\r\n";
    oss << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    oss << "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    for (const auto& [k, v] : extraHeaders) {
        oss << k << ": " << v << "\r\n";
    }
    oss << "\r\n";
    oss << body;
    return oss.str();
}

HttpServer::HttpServer(uint16_t port)
    : port_(port), running_(false), serverFd_(INVALID_SOCKET) {}

HttpServer::~HttpServer() {
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool HttpServer::initWinsock() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

bool HttpServer::start() {
    if (running_) return false;
    if (!initWinsock()) return false;

    serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd_ == INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverFd_, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(serverFd_);
        serverFd_ = INVALID_SOCKET;
        return false;
    }

    if (listen(serverFd_, 128) == SOCKET_ERROR) {
        closesocket(serverFd_);
        serverFd_ = INVALID_SOCKET;
        return false;
    }

    running_ = true;
    serverThread_ = std::thread(&HttpServer::acceptLoop, this);
    return true;
}

void HttpServer::stop() {
    running_ = false;
    if (serverFd_ != INVALID_SOCKET) {
        closesocket(serverFd_);
        serverFd_ = INVALID_SOCKET;
    }
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
}

void HttpServer::acceptLoop() {
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientFd = static_cast<int>(accept(serverFd_, (struct sockaddr*)&clientAddr, &addrLen));
        if (clientFd == INVALID_SOCKET) {
            if (running_) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        std::thread(&HttpServer::handleClient, this, clientFd).detach();
    }
}

void HttpServer::handleClient(int clientFd) {
    std::string raw;
    char buf[4096];
    int n;
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        n = recv(clientFd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            raw += buf;
            if (raw.find("\r\n\r\n") != std::string::npos) break;
        } else if (n == 0) {
            break;
        } else {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > std::chrono::seconds(5)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    HttpResponse res = processRequest(raw);
    std::string responseStr = res.toString();
    send(clientFd, responseStr.data(), static_cast<int>(responseStr.size()), 0);
    closesocket(clientFd);
}

HttpResponse HttpServer::processRequest(const std::string& raw) {
    HttpRequest req;

    // Parse request line
    auto firstLineEnd = raw.find("\r\n");
    if (firstLineEnd == std::string::npos) {
        HttpResponse res;
        res.statusCode = 400;
        res.body = "{\"error\":\"Bad Request\"}";
        return res;
    }
    std::string firstLine = raw.substr(0, firstLineEnd);
    std::istringstream lineStream(firstLine);
    lineStream >> req.method >> req.path;

    // Parse headers
    auto headerEnd = raw.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headerSection = raw.substr(firstLineEnd + 2, headerEnd - firstLineEnd - 2);
        std::istringstream headerStream(headerSection);
        std::string headerLine;
        while (std::getline(headerStream, headerLine)) {
            if (headerLine.empty() || headerLine == "\r") continue;
            if (headerLine.back() == '\r') headerLine.pop_back();
            auto colon = headerLine.find(": ");
            if (colon != std::string::npos) {
                std::string key = headerLine.substr(0, colon);
                std::string val = headerLine.substr(colon + 2);
                req.headers[key] = val;
            }
        }
        req.body = raw.substr(headerEnd + 4);
    }

    return routeRequest(req);
}

HttpResponse HttpServer::routeRequest(const HttpRequest& req) {
    // Handle CORS preflight
    if (req.method == "OPTIONS") {
        HttpResponse res;
        res.statusCode = 204;
        return res;
    }

    // Match routes
    for (const auto& route : routes_) {
        if (route.method != req.method && route.method != "*") continue;

        std::string pattern = route.path;
        std::string reqPath = req.path;

        // Strip query string from request path
        auto qpos = reqPath.find('?');
        std::string queryStr;
        if (qpos != std::string::npos) {
            queryStr = reqPath.substr(qpos + 1);
            reqPath = reqPath.substr(0, qpos);
            // Parse query params
            const_cast<HttpRequest&>(req).queryParams.clear();
            std::istringstream qs(queryStr);
            std::string pair;
            while (std::getline(qs, pair, '&')) {
                auto eq = pair.find('=');
                if (eq != std::string::npos) {
                    const_cast<HttpRequest&>(req).queryParams[pair.substr(0, eq)] = pair.substr(eq + 1);
                }
            }
        }

        // Convert {param} patterns to regex with capture groups
        std::string regexStr;
        std::vector<std::string> paramNames;
        size_t i = 0;
        while (i < pattern.size()) {
            if (pattern[i] == '{') {
                auto end = pattern.find('}', i);
                if (end != std::string::npos) {
                    paramNames.push_back(pattern.substr(i + 1, end - i - 1));
                    regexStr += "([^/]+)";
                    i = end + 1;
                } else {
                    regexStr += pattern[i];
                    ++i;
                }
            } else {
                if (std::strchr(".+*?^$|()[]\\", pattern[i])) {
                    regexStr += '\\';
                }
                regexStr += pattern[i];
                ++i;
            }
        }

        // Check if path matches
        try {
            std::regex routeRegex(regexStr);
            std::smatch routeMatch;
            if (std::regex_match(reqPath, routeMatch, routeRegex)) {
                HttpRequest copy = req;
                copy.pathParams.clear();
                for (size_t j = 1; j < routeMatch.size(); ++j) {
                    if (j - 1 < paramNames.size()) {
                        copy.pathParams[paramNames[j - 1]] = routeMatch[j].str();
                    }
                }
                return route.handler(copy);
            }
        } catch (const std::regex_error&) {
            // If regex fails, skip this route
            continue;
        }
    }

    // 404 Not Found
    HttpResponse res;
    res.statusCode = 404;
    res.body = "{\"error\":\"Not Found\"}";
    return res;
}

void HttpServer::get(const std::string& path, RouteHandler handler) {
    addRoute("GET", path, std::move(handler));
}

void HttpServer::post(const std::string& path, RouteHandler handler) {
    addRoute("POST", path, std::move(handler));
}

void HttpServer::addRoute(const std::string& method, const std::string& path, RouteHandler handler) {
    routes_.push_back({method, path, std::move(handler)});
}

} // namespace freebuff::backend
