#include "backend/AuthMiddleware.h"
#include "auth/JWT.h"
#include <regex>

namespace freebuff::backend {

AuthMiddleware::AuthMiddleware(auth::AuthService& authService)
    : authService_(authService) {}

std::string AuthMiddleware::extractBearerToken(const HttpRequest& req) const {
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return "";

    std::regex bearerRegex("^Bearer[[:space:]]+(.+)$", std::regex::icase);
    std::smatch match;
    if (std::regex_match(it->second, match, bearerRegex)) {
        return match[1].str();
    }
    return "";
}

nlohmann::json AuthMiddleware::makeError(int statusCode, const std::string& message) {
    nlohmann::json j;
    j["status"] = "error";
    j["error"] = message;
    j["status_code"] = statusCode;
    return j;
}

bool AuthMiddleware::isPublicRoute(const std::string& method, const std::string& path) {
    if (method == "GET" && path == "/api/health") return true;
    if (method == "POST" && path == "/api/auth/login") return true;
    if (method == "POST" && path == "/api/auth/register") return true;
    if (method == "OPTIONS") return true;
    return false;
}

std::optional<HttpResponse> AuthMiddleware::requireAuth(const HttpRequest& req) const {
    if (isPublicRoute(req.method, req.path)) {
        return std::nullopt;
    }

    std::string token = extractBearerToken(req);
    if (token.empty()) {
        HttpResponse res;
        res.statusCode = 401;
        res.body = makeError(401, "Missing or invalid Authorization header. Use: Bearer <token>").dump();
        res.extraHeaders["WWW-Authenticate"] = "Bearer realm=\"freebuff\"";
        return res;
    }

    if (!authService_.validateToken(token)) {
        HttpResponse res;
        res.statusCode = 401;
        res.body = makeError(401, "Invalid or expired token. Please login again.").dump();
        res.extraHeaders["WWW-Authenticate"] = "Bearer error=\"invalid_token\"";
        return res;
    }

    return std::nullopt;
}

std::optional<HttpResponse> AuthMiddleware::requireRole(const HttpRequest& req,
                                                         const std::string& role) const {
    auto authResult = requireAuth(req);
    if (authResult.has_value()) return authResult;

    auto payload = getTokenPayload(req);
    if (!payload.has_value()) {
        HttpResponse res;
        res.statusCode = 401;
        res.body = makeError(401, "Invalid token payload").dump();
        return res;
    }

    auto roleIt = payload->find("role");
    if (roleIt == payload->end() || !roleIt->is_string()) {
        HttpResponse res;
        res.statusCode = 403;
        res.body = makeError(403, "Token missing role claim").dump();
        return res;
    }

    std::string userRole = roleIt->get<std::string>();
    if (userRole != role && role != "") {
        HttpResponse res;
        res.statusCode = 403;
        nlohmann::json err = makeError(403, "Insufficient permissions. Required role: " + role);
        err["user_role"] = userRole;
        err["required_role"] = role;
        res.body = err.dump();
        return res;
    }

    return std::nullopt;
}

std::optional<nlohmann::json> AuthMiddleware::getTokenPayload(const HttpRequest& req) const {
    std::string token = extractBearerToken(req);
    if (token.empty()) return std::nullopt;

    return auth::JWT::verify(token, authService_.jwtSecret(), "freebuff");
}

} // namespace freebuff::backend
