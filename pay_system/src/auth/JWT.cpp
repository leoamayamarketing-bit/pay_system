#include "auth/JWT.h"
#include "crypto/HMAC.h"
#include <sstream>
#include <chrono>
#include <regex>

namespace freebuff::auth {

namespace {
const std::string JWT_ALGORITHM = "HS256";
const std::string JWT_TYPE = "JWT";

std::string base64Encode(const std::string& in) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    unsigned int val = 0;
    int valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    return out;
}

std::string base64Decode(const std::string& in) {
    static const std::string b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; ++i) T[b64[i]] = i;
    std::string out;
    out.reserve((in.size() * 3) / 4);
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string toBase64Url(const std::string& base64) {
    std::string out = base64;
    for (auto& c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    auto pos = out.find('=');
    if (pos != std::string::npos) out.erase(pos);
    return out;
}

std::string fromBase64Url(const std::string& base64url) {
    std::string in = base64url;
    for (auto& c : in) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    switch (in.size() % 4) {
        case 2: in += "=="; break;
        case 3: in += "="; break;
    }
    return in;
}
} // namespace

std::string JWT::base64UrlEncode(const std::string& data) {
    return toBase64Url(base64Encode(data));
}

std::string JWT::base64UrlDecode(const std::string& data) {
    return base64Decode(fromBase64Url(data));
}

std::string JWT::createSignature(const std::string& signingInput, const std::string& secret) {
    auto hmac = crypto::HMAC::sha256(secret, signingInput);
    std::string sig(reinterpret_cast<const char*>(hmac.data()), hmac.size());
    return base64UrlEncode(sig);
}

std::string JWT::create(const nlohmann::json& payload, const std::string& secret,
                          const std::string& issuer, long expirationSec) {
    auto now = std::chrono::system_clock::now();
    auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

    nlohmann::json header = {
        {"alg", JWT_ALGORITHM},
        {"typ", JWT_TYPE}
    };

    nlohmann::json claims = payload;
    claims["iss"] = issuer;
    claims["iat"] = nowSec;
    claims["exp"] = nowSec + expirationSec;

    std::string headerEncoded = base64UrlEncode(header.dump());
    std::string payloadEncoded = base64UrlEncode(claims.dump());
    std::string signingInput = headerEncoded + "." + payloadEncoded;
    std::string signature = createSignature(signingInput, secret);

    return signingInput + "." + signature;
}

std::optional<nlohmann::json> JWT::verify(const std::string& token,
                                            const std::string& secret,
                                            const std::string& issuer) {
    auto parts = std::vector<std::string>();
    std::string current;
    for (char c : token) {
        if (c == '.') { parts.push_back(current); current.clear(); }
        else current += c;
    }
    parts.push_back(current);

    if (parts.size() != 3) return std::nullopt;

    std::string signingInput = parts[0] + "." + parts[1];
    std::string expectedSig = createSignature(signingInput, secret);

    if (parts[2] != expectedSig) return std::nullopt;

    try {
        auto decoded = base64UrlDecode(parts[1]);
        auto payload = nlohmann::json::parse(decoded);

        auto now = std::chrono::system_clock::now();
        auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        if (payload.contains("exp") && payload["exp"].get<long>() < nowSec)
            return std::nullopt;

        if (payload.contains("iss") && payload["iss"].get<std::string>() != issuer)
            return std::nullopt;

        return payload;
    } catch (...) {
        return std::nullopt;
    }
}

std::string JWT::extractToken(const std::string& authHeader) {
    std::regex bearerRegex("^Bearer\\s+(.+)$", std::regex::icase);
    std::smatch match;
    if (std::regex_match(authHeader, match, bearerRegex)) {
        return match[1].str();
    }
    return "";
}

} // namespace freebuff::auth
