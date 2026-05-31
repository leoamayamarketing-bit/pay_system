#ifndef FREEBUFF_JWT_H
#define FREEBUFF_JWT_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace freebuff::auth {

class JWT {
public:
    static std::string create(const nlohmann::json& payload, const std::string& secret,
                               const std::string& issuer = "freebuff",
                               long expirationSec = 3600);

    static std::optional<nlohmann::json> verify(const std::string& token,
                                                  const std::string& secret,
                                                  const std::string& issuer = "freebuff");

    static std::string extractToken(const std::string& authHeader);

private:
    static std::string base64UrlEncode(const std::string& data);
    static std::string base64UrlDecode(const std::string& data);
    static std::string createSignature(const std::string& signingInput, const std::string& secret);
};

} // namespace freebuff::auth
#endif
