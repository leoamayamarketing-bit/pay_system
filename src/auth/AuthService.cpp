#include "auth/AuthService.h"
#include "auth/JWT.h"
#include "crypto/SHA256.h"
#include <sstream>
#include <chrono>
#include <algorithm>
#include <random>

namespace freebuff::auth {

AuthService::AuthService()
    : jwtSecret_("freebuff-default-secret-change-in-production-2024")
{
    seedDefaultUsers();
}

AuthService::AuthService(const std::string& jwtSecret)
    : jwtSecret_(jwtSecret)
{
    seedDefaultUsers();
}

std::string AuthService::hashPassword(const std::string& password) const {
    return crypto::SHA256::hash(password + ":freebuff:salt");
}

bool AuthService::verifyPassword(const std::string& password, const std::string& hash) const {
    return hashPassword(password) == hash;
}

void AuthService::seedDefaultUsers() {
    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    users_.push_back({std::string("user-001"), std::string("admin"), std::string("admin@freebuff.com"), std::string("admin"),
                       hashPassword("admin123"), true, static_cast<long>(nowSec)});
    users_.push_back({std::string("user-002"), std::string("merchant1"), std::string("merchant1@freebuff.com"), std::string("merchant"),
                       hashPassword("merchant123"), true, static_cast<long>(nowSec)});
    users_.push_back({std::string("user-003"), std::string("demo"), std::string("demo@freebuff.com"), std::string("merchant"),
                       hashPassword("demo123"), true, static_cast<long>(nowSec)});
    nextUserId_ = 4;
}

AuthResponse AuthService::login(const std::string& username, const std::string& password) {
    AuthResponse response;

    auto user = getUserByUsername(username);
    if (!user) {
        response.error = "Invalid username or password";
        return response;
    }

    if (!user->enabled) {
        response.error = "Account is disabled";
        return response;
    }

    if (!verifyPassword(password, user->passwordHash)) {
        response.error = "Invalid username or password";
        return response;
    }

    nlohmann::json payload;
    payload["sub"] = user->id;
    payload["username"] = user->username;
    payload["role"] = user->role;
    payload["email"] = user->email;

    long expirationSec = 86400; // 24 hours
    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    response.success = true;
    response.token = JWT::create(payload, jwtSecret_, "freebuff", expirationSec);
    response.userId = user->id;
    response.username = user->username;
    response.role = user->role;
    response.expiresAt = nowSec + expirationSec;

    return response;
}

AuthResponse AuthService::registerUser(const std::string& username, const std::string& email,
                                         const std::string& password, const std::string& role) {
    AuthResponse response;

    if (username.empty() || email.empty() || password.empty()) {
        response.error = "Username, email and password are required";
        return response;
    }

    if (password.size() < 6) {
        response.error = "Password must be at least 6 characters";
        return response;
    }

    if (getUserByUsername(username)) {
        response.error = "Username already exists";
        return response;
    }

    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    std::stringstream ss;
    ss << "user-" << std::setw(3) << std::setfill('0') << nextUserId_++;

    User newUser;
    newUser.id = ss.str();
    newUser.username = username;
    newUser.email = email;
    newUser.role = role;
    newUser.passwordHash = hashPassword(password);
    newUser.enabled = true;
    newUser.createdAt = nowSec;

    users_.push_back(newUser);

    nlohmann::json payload;
    payload["sub"] = newUser.id;
    payload["username"] = newUser.username;
    payload["role"] = newUser.role;
    payload["email"] = newUser.email;

    long expirationSec = 86400;
    response.success = true;
    response.token = JWT::create(payload, jwtSecret_, "freebuff", expirationSec);
    response.userId = newUser.id;
    response.username = newUser.username;
    response.role = newUser.role;
    response.expiresAt = nowSec + expirationSec;

    return response;
}

bool AuthService::validateToken(const std::string& token) {
    auto result = JWT::verify(token, jwtSecret_, "freebuff");
    return result.has_value();
}

std::optional<User> AuthService::getUserFromToken(const std::string& token) {
    auto payload = JWT::verify(token, jwtSecret_, "freebuff");
    if (!payload) return std::nullopt;

    std::string userId = (*payload)["sub"].get<std::string>();
    return getUserById(userId);
}

std::optional<User> AuthService::getUserById(const std::string& id) const {
    auto it = std::find_if(users_.begin(), users_.end(),
        [&](const User& u) { return u.id == id; });
    if (it != users_.end()) return *it;
    return std::nullopt;
}

std::optional<User> AuthService::getUserByUsername(const std::string& username) const {
    auto it = std::find_if(users_.begin(), users_.end(),
        [&](const User& u) { return u.username == username; });
    if (it != users_.end()) return *it;
    return std::nullopt;
}

std::vector<User> AuthService::listUsers() const {
    return users_;
}

bool AuthService::deleteUser(const std::string& id) {
    auto it = std::find_if(users_.begin(), users_.end(),
        [&](const User& u) { return u.id == id; });
    if (it != users_.end()) {
        users_.erase(it);
        return true;
    }
    return false;
}

} // namespace freebuff::auth
