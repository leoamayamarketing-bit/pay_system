#ifndef FREEBUFF_MERCADOPAGO_H
#define FREEBUFF_MERCADOPAGO_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace freebuff::backend {

struct MercadoPagoConfig {
    std::string accessToken;
    std::string webhookSecret;
    bool sandboxMode{true};
};

class MercadoPago {
public:
    explicit MercadoPago(MercadoPagoConfig config = {});

    nlohmann::json processWebhook(const nlohmann::json& body,
                                   const std::string& signature = "");

    nlohmann::json createPayment(const nlohmann::json& paymentData);
    nlohmann::json getPayment(const std::string& paymentId);
    nlohmann::json cancelPayment(const std::string& paymentId);
    nlohmann::json refundPayment(const std::string& paymentId, double amount = 0);

    void setConfig(const MercadoPagoConfig& config) { config_ = config; }
    const MercadoPagoConfig& config() const { return config_; }

private:
    MercadoPagoConfig config_;
    std::vector<nlohmann::json> paymentHistory_;
    int nextPaymentId_{10000000};

    std::string generatePaymentLink(const std::string& paymentId) const;
    bool verifySignature(const std::string& body, const std::string& signature) const;
};

} // namespace freebuff::backend
#endif
