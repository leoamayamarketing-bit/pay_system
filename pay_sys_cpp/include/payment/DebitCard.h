#ifndef FREEBUFF_DEBITCARD_H
#define FREEBUFF_DEBITCARD_H

#include "core/IPaymentMethod.h"
#include "core/Money.h"
#include <string>

namespace freebuff {

struct BankAccount {
    std::string accountNumber;
    std::string bankCode;
    Money balance;
};

class DebitCard : public IPaymentMethod {
public:
    DebitCard(std::string cardNumber, BankAccount account);

    [[nodiscard]] std::string name() const noexcept override;
    [[nodiscard]] PaymentResult validate() const noexcept override;
    [[nodiscard]] PaymentResult process(const Money& amount) noexcept override;
    [[nodiscard]] PaymentResult refund(const std::string& transactionId) noexcept override;
    [[nodiscard]] std::string description() const noexcept override;

    void setBalance(const Money& balance) noexcept { account_.balance = balance; }
    [[nodiscard]] const Money& balance() const noexcept { return account_.balance; }

private:
    std::string cardNumber_;
    BankAccount account_;
    bool processed_{false};
    std::string lastTransactionId_;
};

} // namespace freebuff

#endif
