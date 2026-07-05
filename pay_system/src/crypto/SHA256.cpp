#include "crypto/SHA256.h"
#include <cstring>

namespace freebuff::crypto {

namespace {
const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t sigma0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
inline uint32_t sigma1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
inline uint32_t gamma0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
inline uint32_t gamma1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }
} // namespace

SHA256::SHA256() {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
    bitCount_ = 0;
    bufferLen_ = 0;
    finalized_ = false;
}

void SHA256::update(const uint8_t* data, size_t length) {
    if (finalized_) return;
    for (size_t i = 0; i < length; ++i) {
        buffer_[bufferLen_++] = data[i];
        if (bufferLen_ == 64) {
            transform(buffer_);
            bitCount_ += 512;
            bufferLen_ = 0;
        }
    }
}

void SHA256::update(const std::string& data) {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

void SHA256::transform(const uint8_t block[64]) {
    uint32_t W[64];
    for (int i = 0; i < 16; ++i) {
        W[i] = (static_cast<uint32_t>(block[i*4]) << 24) |
               (static_cast<uint32_t>(block[i*4+1]) << 16) |
               (static_cast<uint32_t>(block[i*4+2]) << 8) |
               (static_cast<uint32_t>(block[i*4+3]));
    }
    for (int i = 16; i < 64; ++i) {
        W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];
    }

    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
        uint32_t t2 = sigma0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
}

void SHA256::finalize() {
    if (finalized_) return;

    uint64_t bits = bitCount_ + bufferLen_ * 8;
    buffer_[bufferLen_++] = 0x80;

    if (bufferLen_ > 56) {
        while (bufferLen_ < 64) buffer_[bufferLen_++] = 0;
        transform(buffer_);
        bufferLen_ = 0;
    }

    while (bufferLen_ < 56) buffer_[bufferLen_++] = 0;

    for (int i = 0; i < 8; ++i) {
        buffer_[56 + i] = static_cast<uint8_t>((bits >> (56 - i * 8)) & 0xFF);
    }
    transform(buffer_);
    finalized_ = true;
}

std::array<uint8_t, 32> SHA256::digest() {
    finalize();
    std::array<uint8_t, 32> result;
    for (int i = 0; i < 8; ++i) {
        result[i*4]   = static_cast<uint8_t>((state_[i] >> 24) & 0xFF);
        result[i*4+1] = static_cast<uint8_t>((state_[i] >> 16) & 0xFF);
        result[i*4+2] = static_cast<uint8_t>((state_[i] >> 8) & 0xFF);
        result[i*4+3] = static_cast<uint8_t>(state_[i] & 0xFF);
    }
    return result;
}

std::string SHA256::hexDigest() {
    auto d = digest();
    const char* hex = "0123456789abcdef";
    std::string result;
    result.reserve(64);
    for (uint8_t byte : d) {
        result += hex[(byte >> 4) & 0xF];
        result += hex[byte & 0xF];
    }
    return result;
}

std::string SHA256::hash(const std::string& data) {
    SHA256 sha;
    sha.update(data);
    return sha.hexDigest();
}

std::array<uint8_t, 32> SHA256::hashBytes(const std::string& data) {
    SHA256 sha;
    sha.update(data);
    return sha.digest();
}

} // namespace freebuff::crypto
