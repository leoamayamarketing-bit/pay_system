#ifndef FREEBUFF_AUTHSERVICE_H
#define FREEBUFF_AUTHSERVICE_H

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace freebuff::auth {

struct User {
    std::string id;
    std::string username;
    std::string email;
    std::string role; // "admin" or "merchant"
    std::string passwordHash;
    bool enabled{true};
    long createdAt{0};
};

struct AuthResponse {
    bool success{false};
    std::string token;
    std::string userId;
    std::string username;
    std::string role;
    std::string error;
    long expiresAt{0};
};

class AuthService {
public:
    AuthService();
    explicit AuthService(const std::string& jwtSecret);

    AuthResponse login(const std::string& username, const std::string& password);
    AuthResponse registerUser(const std::string& username, const std::string& email,
                               const std::string& password, const std::string& role = "merchant");
    bool validateToken(const std::string& token);
    std::optional<User> getUserFromToken(const std::string& token);
    std::optional<User> getUserById(const std::string& id) const;
    std::optional<User> getUserByUsername(const std::string& username) const;
    std::vector<User> listUsers() const;
    bool deleteUser(const std::string& id);

    const std::string& jwtSecret() const { return jwtSecret_; }

private:
    std::string jwtSecret_;
    std::vector<User> users_;
    long nextUserId_{1};

    std::string hashPassword(const std::string& password) const;
    bool verifyPassword(const std::string& password, const std::string& hash) const;
    void seedDefaultUsers();
};

} // namespace freebuff::auth
#endif
