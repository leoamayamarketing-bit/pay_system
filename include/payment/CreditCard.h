#ifndef FREEBUFF_CREDITCARD_H
#define FREEBUFF_CREDITCARD_H

#include "core/IPaymentMethod.h"
#include <string>

namespace freebuff {

struct CardInfo {
    std::string cardNumber;
    int expiryMonth{0};
    int expiryYear{0};
    std::string cvv;
    std::string cardHolderName;
};

class CreditCard : public IPaymentMethod {
public:
    explicit CreditCard(CardInfo info);

    [[nodiscard]] std::string name() const noexcept override;
    [[nodiscard]] PaymentResult validate() const noexcept override;
    [[nodiscard]] PaymentResult process(const Money& amount) noexcept override;
    [[nodiscard]] PaymentResult refund(const std::string& transactionId) noexcept override;
    [[nodiscard]] std::string description() const noexcept override;

    static bool validateLuhn(const std::string& cardNumber) noexcept;
    static bool validateCVV(const std::string& cvv) noexcept;

    [[nodiscard]] const CardInfo& cardInfo() const noexcept { return info_; }

private:
    CardInfo info_;
    bool processed_{false};
    std::string lastTransactionId_;
};

} // namespace freebuff

#endif
