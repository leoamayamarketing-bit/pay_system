#ifndef FREEBUFF_AUTHMIDDLEWARE_H
#define FREEBUFF_AUTHMIDDLEWARE_H

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

#include "backend/HttpServer.h"
#include "auth/AuthService.h"

namespace freebuff::backend {

class AuthMiddleware {
public:
    explicit AuthMiddleware(auth::AuthService& authService);

    // Returns nullopt if authorized, error HttpResponse if unauthorized (401/403)
    std::optional<HttpResponse> requireAuth(const HttpRequest& req) const;

    // Validates JWT and checks that user has the required role
    std::optional<HttpResponse> requireRole(const HttpRequest& req,
                                             const std::string& role) const;

    // Extract and return the validated JWT payload (call after requireAuth passes)
    std::optional<nlohmann::json> getTokenPayload(const HttpRequest& req) const;

    // List of routes that don't require authentication
    static bool isPublicRoute(const std::string& method, const std::string& path);

private:
    auth::AuthService& authService_;

    std::string extractBearerToken(const HttpRequest& req) const;
    static nlohmann::json makeError(int statusCode, const std::string& message);
};

} // namespace freebuff::backend

#endif // FREEBUFF_AUTHMIDDLEWARE_H
