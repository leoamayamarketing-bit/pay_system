#ifndef FREEBUFF_QRPAYMENT_H
#define FREEBUFF_QRPAYMENT_H

#include "core/IPaymentMethod.h"
#include <string>
#include <vector>

namespace freebuff {

struct QRPayload {
    std::string merchantId;
    std::string merchantName;
    std::string reference;
    Money amount;

    [[nodiscard]] std::string encode() const;
    static std::optional<QRPayload> decode(const std::string& data);
};

class QRPayment : public IPaymentMethod {
public:
    explicit QRPayment(QRPayload payload);

    [[nodiscard]] std::string name() const noexcept override;
    [[nodiscard]] PaymentResult validate() const noexcept override;
    [[nodiscard]] PaymentResult process(const Money& amount) noexcept override;
    [[nodiscard]] PaymentResult refund(const std::string& transactionId) noexcept override;
    [[nodiscard]] std::string description() const noexcept override;

    [[nodiscard]] std::vector<std::string> generateQRCode() const;

private:
    QRPayload payload_;
    bool processed_{false};
    std::string lastTransactionId_;
};

} // namespace freebuff

#endif
