#ifndef FREEBUFF_NFCPAYMENT_H
#define FREEBUFF_NFCPAYMENT_H

#include "core/IPaymentMethod.h"
#include <string>
#include <vector>

namespace freebuff {

enum class NFCCardType {
    Visa,
    Mastercard,
    AmericanExpress,
    Unknown
};

struct NFCData {
    std::string cardUid;
    NFCCardType cardType{NFCCardType::Unknown};
    std::string maskedPan;
};

class NFCPayment : public IPaymentMethod {
public:
    explicit NFCPayment(NFCData nfcData);

    [[nodiscard]] std::string name() const noexcept override;
    [[nodiscard]] PaymentResult validate() const noexcept override;
    [[nodiscard]] PaymentResult process(const Money& amount) noexcept override;
    [[nodiscard]] PaymentResult refund(const std::string& transactionId) noexcept override;
    [[nodiscard]] std::string description() const noexcept override;

    [[nodiscard]] const NFCData& nfcData() const noexcept { return nfcData_; }

private:
    NFCData nfcData_;
    bool processed_{false};
    std::string lastTransactionId_;
};

} // namespace freebuff

#endif
