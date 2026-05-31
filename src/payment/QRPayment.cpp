#include "payment/QRPayment.h"
#include "qr_sim/QRSimulator.h"
#include <sstream>
#include <random>
#include <algorithm>
#include <iomanip>

namespace freebuff {

// ── QRPayload forwarding implementations ──────────────────────────────────
// These delegate to the shared qr_sim library's QRPayload to avoid duplication.

std::string QRPayload::encode() const {
    qr::QRPayload p{merchantId, merchantName, reference, amount};
    return p.encode();
}

std::optional<QRPayload> QRPayload::decode(const std::string& data) {
    auto decoded = qr::QRPayload::decode(data);
    if (!decoded) return std::nullopt;
    QRPayload p;
    p.merchantId   = decoded->merchantId;
    p.merchantName = decoded->merchantName;
    p.reference    = decoded->reference;
    p.amount       = decoded->amount;
    return p;
}

// ── QRPayment implementation ──────────────────────────────────────────────

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
    // Delegate to the shared QR simulation library
    qr::QRPayload simPayload{payload_.merchantId, payload_.merchantName,
                              payload_.reference, payload_.amount};
    return qr::QRSimulator::generateFromPayload(simPayload);
}

} // namespace freebuff
