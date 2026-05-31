#include "payment/NFCPayment.h"
#include <sstream>
#include <random>

namespace freebuff {

NFCPayment::NFCPayment(NFCData nfcData) : nfcData_(std::move(nfcData)) {}

std::string NFCPayment::name() const noexcept {
    return "NFC Payment";
}

PaymentResult NFCPayment::validate() const noexcept {
    if (nfcData_.cardUid.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "NFC card UID is empty");
    }
    
    if (nfcData_.cardType == NFCCardType::Unknown) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Unknown NFC card type");
    }
    
    return PaymentResult::success("PRE_VALIDATION");
}

PaymentResult NFCPayment::process(const Money& amount) noexcept {
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
    ss << "NFC-";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    
    lastTransactionId_ = ss.str();
    processed_ = true;
    
    return PaymentResult::success(lastTransactionId_);
}

PaymentResult NFCPayment::refund(const std::string& transactionId) noexcept {
    if (!processed_) {
        return PaymentResult::failure(PaymentStatus::RefundFailed,
            "No previous transaction to refund");
    }
    return PaymentResult{
        PaymentStatus::Refunded,
        "NFC Payment refunded: " + transactionId,
        "REF-" + transactionId,
        std::chrono::system_clock::now()
    };
}

std::string NFCPayment::description() const noexcept {
    std::string typeStr;
    switch (nfcData_.cardType) {
        case NFCCardType::Visa:          typeStr = "Visa"; break;
        case NFCCardType::Mastercard:    typeStr = "Mastercard"; break;
        case NFCCardType::AmericanExpress: typeStr = "Amex"; break;
        default: typeStr = "Unknown";
    }
    return "NFC Payment [" + typeStr + "] UID: " + nfcData_.cardUid +
           " PAN: " + nfcData_.maskedPan;
}

} // namespace freebuff
