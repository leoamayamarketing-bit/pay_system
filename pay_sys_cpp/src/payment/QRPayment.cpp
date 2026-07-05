#include "payment/QRPayment.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iomanip>

namespace freebuff {

std::string QRPayload::encode() const {
    std::ostringstream oss;
    oss << "FREEBUFFQR:" << merchantId
        << ":" << merchantName
        << ":" << reference
        << ":" << amount.cents();
    return oss.str();
}

std::optional<QRPayload> QRPayload::decode(const std::string& data) {
    const std::string prefix = "FREEBUFFQR:";
    if (data.find(prefix) != 0) return std::nullopt;
    
    std::string rest = data.substr(prefix.size());
    std::vector<std::string> parts;
    std::string current;
    for (char c : rest) {
        if (c == ':') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty()) parts.push_back(current);
    
    if (parts.size() < 4) return std::nullopt;
    
    QRPayload payload;
    payload.merchantId = parts[0];
    payload.merchantName = parts[1];
    payload.reference = parts[2];
    try {
        payload.amount = Money::fromCents(std::stoll(parts[3]));
    } catch (...) {
        return std::nullopt;
    }
    
    return payload;
}

QRPayment::QRPayment(QRPayload payload) : payload_(std::move(payload)) {}

std::string QRPayment::name() const noexcept {
    return "QR Payment";
}

PaymentResult QRPayment::validate() const noexcept {
    if (payload_.merchantId.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Merchant ID is empty");
    }
    if (payload_.merchantName.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Merchant name is empty");
    }
    return PaymentResult::success("PRE_VALIDATION");
}

PaymentResult QRPayment::process(const Money& amount) noexcept {
    if (amount.isNegative() || amount.isZero()) {
        return PaymentResult::failure(PaymentStatus::InvalidAmount,
            "Amount must be positive");
    }
    
    auto validation = validate();
    if (!validation.isSuccess()) {
        return validation;
    }
    
    // Generate transaction ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "QR-";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    
    payload_.amount = amount;
    lastTransactionId_ = ss.str();
    processed_ = true;
    
    return PaymentResult::success(lastTransactionId_);
}

PaymentResult QRPayment::refund(const std::string& transactionId) noexcept {
    if (!processed_) {
        return PaymentResult::failure(PaymentStatus::RefundFailed,
            "No previous transaction to refund");
    }
    return PaymentResult{
        PaymentStatus::Refunded,
        "QR Payment refunded: " + transactionId,
        "REF-" + transactionId,
        std::chrono::system_clock::now()
    };
}

std::string QRPayment::description() const noexcept {
    return "QR Payment to " + payload_.merchantName +
           " (Ref: " + payload_.reference + ")";
}

std::vector<std::string> QRPayment::generateQRCode() const {
    std::string data = payload_.encode();
    std::vector<std::string> qrCode;
    
    // Simplified ASCII QR code representation
    // In production, this would use libqrencode
    int size = 21; // Version 1 QR code
    
    // Create a simple visual representation
    std::string border(size + 2, '#');
    qrCode.push_back(border);
    
    for (int y = 0; y < size; ++y) {
        std::string line = "#";
        for (int x = 0; x < size; ++x) {
            // Create finder patterns (corners)
            if ((y < 7 && x < 7) || (y < 7 && x >= size - 7) || (y >= size - 7 && x < 7)) {
                if ((x == 0 || x == 6 || y == 0 || y == 6) ||
                    (x >= 2 && x <= 4 && y >= 2 && y <= 4)) {
                    line += '#';
                } else {
                    line += ' ';
                }
            } else {
                // Data area with pseudo-random pattern based on content
                int idx = (y * size + x) % data.size();
                char c = data[idx];
                if (c % 3 == 0) line += '#';
                else if (c % 3 == 1) line += '+';
                else line += ' ';
            }
        }
        line += "#";
        qrCode.push_back(line);
    }
    
    qrCode.push_back(border);
    return qrCode;
}

} // namespace freebuff
