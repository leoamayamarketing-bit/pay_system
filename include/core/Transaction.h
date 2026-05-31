#ifndef FREEBUFF_TRANSACTION_H
#define FREEBUFF_TRANSACTION_H

#include <string>
#include <chrono>
#include <ostream>
#include "Money.h"
#include "PaymentResult.h"

namespace freebuff {

struct Transaction {
    std::string id;
    std::string paymentMethod;
    Money amount;
    PaymentStatus status{PaymentStatus::UnknownError};
    std::string description;
    std::chrono::system_clock::time_point timestamp;

    [[nodiscard]] std::string toString() const;
};

} // namespace freebuff

#endif // FREEBUFF_TRANSACTION_H
