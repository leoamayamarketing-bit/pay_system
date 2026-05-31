#include "payment/DebitCard.h"
#include <sstream>
#include <random>
#include <iomanip>

namespace freebuff {

DebitCard::DebitCard(std::string cardNumber, BankAccount account)
    : cardNumber_(std::move(cardNumber))
    , account_(std::move(account))
{}

std::string DebitCard::name() const noexcept {
    return "Debit Card";
}

PaymentResult DebitCard::validate() const noexcept {
    if (cardNumber_.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Card number is empty");
    }
    
    if (account_.accountNumber.empty()) {
        return PaymentResult::failure(PaymentStatus::InvalidCard,
            "Bank account number is empty");
    }
    
    return PaymentResult::success("PRE_VALIDATION");
}

PaymentResult DebitCard::process(const Money& amount) noexcept {
    if (amount.isNegative() || amount.isZero()) {
        return PaymentResult::failure(PaymentStatus::InvalidAmount,
            "Amount must be positive");
    }
    
    auto validation = validate();
    if (!validation.isSuccess()) {
        return validation;
    }
    
    // Verify sufficient balance
    if (account_.balance < amount) {
        return PaymentResult::failure(PaymentStatus::InsufficientFunds,
            "Insufficient balance: available " + account_.balance.toString() +
            ", required " + amount.toString());
    }
    
    // Deduct balance
    account_.balance = account_.balance - amount;
    
    // Generate transaction ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "DB-";
    for (int i = 0; i < 12; ++i) {
        ss << std::hex << dis(gen);
    }
    
    lastTransactionId_ = ss.str();
    processed_ = true;
    
    return PaymentResult::success(lastTransactionId_);
}

PaymentResult DebitCard::refund(const std::string& transactionId) noexcept {
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

std::string DebitCard::description() const noexcept {
    std::string masked = cardNumber_;
    if (masked.size() > 4) {
        masked = std::string(masked.size() - 4, '*') + masked.substr(masked.size() - 4);
    }
    return "Debit Card [" + masked + "] Bank: " + account_.bankCode +
           " Balance: " + account_.balance.toString();
}

} // namespace freebuff
