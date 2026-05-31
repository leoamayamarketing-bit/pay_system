#ifndef FREEBUFF_PAYMENTRESULT_H
#define FREEBUFF_PAYMENTRESULT_H

#include <string>
#include <optional>
#include <chrono>

namespace freebuff {

enum class PaymentStatus {
    Success,
    InsufficientFunds,
    InvalidCard,
    ExpiredCard,
    InvalidCVV,
    InvalidAmount,
    NetworkError,
    HardwareError,
    Refunded,
    RefundFailed,
    UnknownError
};

struct PaymentResult {
    PaymentStatus status{PaymentStatus::UnknownError};
    std::string message;
    std::optional<std::string> transactionId;
    std::chrono::system_clock::time_point timestamp;

    [[nodiscard]] bool isSuccess() const noexcept {
        return status == PaymentStatus::Success;
    }

    [[nodiscard]] static PaymentResult success(const std::string& txId) {
        return PaymentResult{
            PaymentStatus::Success,
            "Transaction completed successfully",
            txId,
            std::chrono::system_clock::now()
        };
    }

    [[nodiscard]] static PaymentResult failure(PaymentStatus s, const std::string& msg) {
        return PaymentResult{
            s,
            msg,
            std::nullopt,
            std::chrono::system_clock::now()
        };
    }
};

std::string paymentStatusToString(PaymentStatus status);

} // namespace freebuff

#endif // FREEBUFF_PAYMENTRESULT_H
