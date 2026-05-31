#ifndef FREEBUFF_HTTPSERVER_H
#define FREEBUFF_HTTPSERVER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdint>
#include <regex>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace freebuff { namespace backend {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::map<std::string, std::string> pathParams;
};

struct HttpResponse {
    int statusCode{200};
    std::string body;
    std::string contentType{"application/json"};
    std::map<std::string, std::string> extraHeaders;

    std::string toString() const;
};

class HttpServer {
public:
    using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;

    explicit HttpServer(uint16_t port = 8080);
    ~HttpServer();

    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void addRoute(const std::string& method, const std::string& path, RouteHandler handler);

    bool start();
    void stop();
    bool isRunning() const { return running_; }
    uint16_t port() const { return port_; }

private:
    uint16_t port_;
    std::atomic<bool> running_{false};

#ifdef _WIN32
    SOCKET serverFd_{INVALID_SOCKET};
#else
    int serverFd_{-1};
    static constexpr int INVALID_SOCKET = -1;
#endif

    std::thread serverThread_;

    struct Route {
        std::string method;
        std::string path;
        RouteHandler handler;
    };
    std::vector<Route> routes_;

    bool initWinsock();
    void acceptLoop();
    void handleClient(int clientFd);
    HttpResponse processRequest(const std::string& raw);
    HttpResponse routeRequest(const HttpRequest& req);
};

}} // namespace freebuff::backend
#endif
