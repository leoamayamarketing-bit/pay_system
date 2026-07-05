#ifndef FREEBUFF_QRSIMULATOR_H
#define FREEBUFF_QRSIMULATOR_H

#include <string>
#include <vector>
#include <optional>
#include "core/Money.h"

namespace freebuff::qr {

struct QRPayload {
    std::string merchantId;
    std::string merchantName;
    std::string reference;
    Money       amount;

    std::string encode() const;
    static std::optional<QRPayload> decode(const std::string& data);
};

class QRSimulator {
public:
    static std::vector<std::string> generate(const std::string& encodedData, int version = 1);
    static std::vector<std::string> generateFromPayload(const QRPayload& payload);
    static bool verify(const std::string& encodedData);
};

} // namespace freebuff::qr
#endif // FREEBUFF_QRSIMULATOR_H
