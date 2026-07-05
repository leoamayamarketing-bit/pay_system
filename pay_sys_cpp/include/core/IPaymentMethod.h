#ifndef FREEBUFF_IPAYMENTMETHOD_H
#define FREEBUFF_IPAYMENTMETHOD_H

#include <memory>
#include <string>
#include <optional>
#include "PaymentResult.h"
#include "Money.h"

namespace freebuff {

class IPaymentMethod {
public:
    virtual ~IPaymentMethod() = default;

    [[nodiscard]] virtual std::string name() const noexcept = 0;
    [[nodiscard]] virtual PaymentResult validate() const noexcept = 0;
    [[nodiscard]] virtual PaymentResult process(const Money& amount) noexcept = 0;
    [[nodiscard]] virtual PaymentResult refund(const std::string& transactionId) noexcept = 0;
    [[nodiscard]] virtual std::string description() const noexcept = 0;
};

} // namespace freebuff

#endif
