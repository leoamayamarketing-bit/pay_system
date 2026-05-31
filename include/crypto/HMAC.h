#ifndef FREEBUFF_HMAC_H
#define FREEBUFF_HMAC_H

#include "SHA256.h"
#include <array>
#include <string>

namespace freebuff::crypto {

class HMAC {
public:
    static std::array<uint8_t, 32> sha256(const std::string& key, const std::string& data);
    static std::array<uint8_t, 32> sha256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data);
    static std::string sha256Hex(const std::string& key, const std::string& data);
};

} // namespace freebuff::crypto
#endif
