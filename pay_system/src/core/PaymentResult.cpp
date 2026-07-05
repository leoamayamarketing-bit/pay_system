#include "core/PaymentResult.h"

namespace freebuff {

std::string paymentStatusToString(PaymentStatus status) {
    switch (status) {
        case PaymentStatus::Success:           return "Success";
        case PaymentStatus::InsufficientFunds: return "Insufficient Funds";
        case PaymentStatus::InvalidCard:       return "Invalid Card";
        case PaymentStatus::ExpiredCard:       return "Expired Card";
        case PaymentStatus::InvalidCVV:        return "Invalid CVV";
        case PaymentStatus::InvalidAmount:     return "Invalid Amount";
        case PaymentStatus::NetworkError:      return "Network Error";
        case PaymentStatus::HardwareError:     return "Hardware Error";
        case PaymentStatus::Refunded:          return "Refunded";
        case PaymentStatus::RefundFailed:      return "Refund Failed";
        case PaymentStatus::UnknownError:      return "Unknown Error";
    }
    return "Unknown";
}

} // namespace freebuff
