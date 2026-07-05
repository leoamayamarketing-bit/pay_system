#include "crypto/HMAC.h"
#include <cstring>
#include <sstream>
#include <iomanip>

namespace freebuff::crypto {

std::array<uint8_t, 32> HMAC::sha256(const std::string& key, const std::string& data) {
    std::vector<uint8_t> keyBytes(key.begin(), key.end());
    std::vector<uint8_t> dataBytes(data.begin(), data.end());
    return sha256(keyBytes, dataBytes);
}

std::array<uint8_t, 32> HMAC::sha256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> k = key;
    if (k.size() > 64) {
        SHA256 sha;
        sha.update(k.data(), k.size());
        auto d = sha.digest();
        k.assign(d.begin(), d.end());
    }
    if (k.size() < 64) {
        k.resize(64, 0);
    }

    std::vector<uint8_t> ipad(64), opad(64);
    for (int i = 0; i < 64; ++i) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }

    SHA256 inner;
    inner.update(ipad.data(), ipad.size());
    inner.update(data.data(), data.size());
    auto innerDigest = inner.digest();

    SHA256 outer;
    outer.update(opad.data(), opad.size());
    outer.update(innerDigest.data(), innerDigest.size());
    return outer.digest();
}

std::string HMAC::sha256Hex(const std::string& key, const std::string& data) {
    auto d = sha256(key, data);
    std::ostringstream oss;
    for (uint8_t byte : d) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

} // namespace freebuff::crypto
