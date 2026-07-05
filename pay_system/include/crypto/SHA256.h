#ifndef FREEBUFF_SHA256_H
#define FREEBUFF_SHA256_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace freebuff::crypto {

class SHA256 {
public:
    SHA256();
    void update(const uint8_t* data, size_t length);
    void update(const std::string& data);
    std::array<uint8_t, 32> digest();
    std::string hexDigest();
    static std::string hash(const std::string& data);
    static std::array<uint8_t, 32> hashBytes(const std::string& data);

private:
    void transform(const uint8_t block[64]);
    void finalize();
    uint64_t bitCount_{0};
    uint32_t state_[8];
    uint8_t buffer_[64];
    size_t bufferLen_{0};
    bool finalized_{false};
};

} // namespace freebuff::crypto
#endif
