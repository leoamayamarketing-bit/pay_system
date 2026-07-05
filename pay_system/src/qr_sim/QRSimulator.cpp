#include "qr_sim/QRSimulator.h"
#include <sstream>
#include <algorithm>

namespace freebuff::qr {

std::string QRPayload::encode() const {
    std::ostringstream oss;
    oss << "FREEBUFFQR:" << merchantId << ":" << merchantName
        << ":" << reference << ":" << amount.cents();
    return oss.str();
}

std::optional<QRPayload> QRPayload::decode(const std::string& data) {
    const std::string prefix = "FREEBUFFQR:";
    if (data.find(prefix) != 0) return std::nullopt;
    std::string rest = data.substr(prefix.size());
    std::vector<std::string> parts;
    std::string current;
    for (char c : rest) {
        if (c == ':') { parts.push_back(current); current.clear(); }
        else { current += c; }
    }
    if (!current.empty()) parts.push_back(current);
    if (parts.size() < 4) return std::nullopt;
    QRPayload payload;
    payload.merchantId   = parts[0];
    payload.merchantName = parts[1];
    payload.reference    = parts[2];
    try { payload.amount = Money::fromCents(std::stoll(parts[3])); }
    catch (...) { return std::nullopt; }
    return payload;
}

std::vector<std::string> QRSimulator::generate(const std::string& encodedData, int version) {
    int size = 21 + (version - 1) * 4;
    if (size < 21) size = 21;
    std::vector<std::string> qrCode;
    std::string border(static_cast<size_t>(size) + 2, '#');
    qrCode.push_back(border);
    for (int y = 0; y < size; ++y) {
        std::string line = "#";
        for (int x = 0; x < size; ++x) {
            bool inFinder = (y < 7 && x < 7) || (y < 7 && x >= size - 7) || (y >= size - 7 && x < 7);
            if (inFinder) {
                if ((x == 0 || x == 6 || y == 0 || y == 6) || (x >= 2 && x <= 4 && y >= 2 && y <= 4))
                    line += '#';
                else
                    line += ' ';
            } else {
                int idx = (y * size + x) % static_cast<int>(encodedData.size());
                char c = encodedData[static_cast<size_t>(idx)];
                if (c % 3 == 0)      line += '#';
                else if (c % 3 == 1)  line += '+';
                else                  line += ' ';
            }
        }
        line += "#";
        qrCode.push_back(line);
    }
    qrCode.push_back(border);
    return qrCode;
}

std::vector<std::string> QRSimulator::generateFromPayload(const QRPayload& payload) {
    return generate(payload.encode());
}

bool QRSimulator::verify(const std::string& encodedData) {
    return QRPayload::decode(encodedData).has_value();
}

} // namespace freebuff::qr
