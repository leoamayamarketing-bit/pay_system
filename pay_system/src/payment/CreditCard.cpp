#include "payment/CreditCard.h"
#include <algorithm>
#include <sstream>
#include <random>

namespace freebuff {

CreditCard::CreditCard(CardInfo info) : info_(std::move(info)) {}

std::string CreditCard::name() const noexcept {
    return "Credit Card";
}

bool CreditCard::validateLuhn(const std::string& cardNumber) noexcept {
    if (cardNumber.empty()) return false;
    
    int sum = 0;
    bool alternate = false;
    
    for (int i = static_cast<int>(cardNumber.size()) - 1; i >= 0; --i) {
        if (!std::isdigit(cardNumber[i])) return false;
        
        int digit = cardNumber[i] - '0';
        if (alternate) {
            digit *= 2;
            if (digit > 9) digit -= 9;
        }
        sum += digit;
        alternate = !alternate;
    }
    
    return (sum % 10) == 0;
}

bool CreditCard::validateCVV(const std::string& cvv) noexcept {
    if (cvv.empty()) return false;
    
    bool validLength = (cvv.size() == 3 || cvv.size() == 4);
    bool allDigits = std::all_of(cvv.begin(), cvv.end(), ::isdigit);
    
    return validLength && allDigits;
}

PaymentResult CreditCard::validate() const noexcept {
    // Check card number is not empty
    if (info_.cardNumber.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Card number is empty");
    }
    
    // Remove spaces and dashes for validation
    std::string cleanNumber;
    std::copy_if(info_.cardNumber.begin(), info_.cardNumber.end(),
                 std::back_inserter(cleanNumber), ::isdigit);
    
    if (cleanNumber.size() < 13 || cleanNumber.size() > 19) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Card number must be between 13 and 19 digits");
    }
    
    if (!validateLuhn(cleanNumber)) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Card number failed Luhn validation");
    }
    
    // Validate expiration date
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm;
    localtime_s(&nowTm, &nowTime);
    
    int currentYear = nowTm.tm_year + 1900;
    int currentMonth = nowTm.tm_mon + 1;
    
    if (info_.expiryYear < currentYear || 
        (info_.expiryYear == currentYear && info_.expiryMonth < currentMonth)) {
        return PaymentResult::failure(PaymentStatus::ExpiredCard,
            "Card has expired");
    }
    
    // Validate CVV
    if (!validateCVV(info_.cvv)) {
        return PaymentResult::failure(PaymentStatus::InvalidCVV,
            "CVV must be 3 or 4 digits");
    }
    
    return PaymentResult::success("PRE_VALIDATION");
}

PaymentResult CreditCard::process(const Money& amount) noexcept {
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
    ss << "CC-";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    
    lastTransactionId_ = ss.str();
    processed_ = true;
    
    return PaymentResult::success(lastTransactionId_);
}

PaymentResult CreditCard::refund(const std::string& transactionId) noexcept {
    if (!processed_) {
        return PaymentResult::failure(PaymentStatus::RefundFailed,
            "No previous transaction to refund");
    }
    
    return PaymentResult{
        PaymentStatus::Refunded,
        "Refund processed for transaction " + transactionId,
        "REF-" + transactionId,
        std::chrono::system_clock::now()
    };
}

std::string CreditCard::description() const noexcept {
    std::string masked = info_.cardNumber;
    if (masked.size() > 4) {
        masked = std::string(masked.size() - 4, '*') + masked.substr(masked.size() - 4);
    }
    return "Credit Card [" + masked + "] exp " +
           std::to_string(info_.expiryMonth) + "/" +
           std::to_string(info_.expiryYear);
}

} // namespace freebuff
