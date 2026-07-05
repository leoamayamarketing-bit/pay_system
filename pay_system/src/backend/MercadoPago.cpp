#include "backend/MercadoPago.h"
#include "crypto/SHA256.h"
#include <sstream>
#include <chrono>
#include <random>
#include <iomanip>

namespace freebuff::backend {

MercadoPago::MercadoPago(MercadoPagoConfig config) : config_(std::move(config)) {}

std::string MercadoPago::generatePaymentLink(const std::string& paymentId) const {
    std::string baseUrl = config_.sandboxMode
        ? "https://sandbox.mercadopago.com.ar/checkout/v1/redirect?pref_id="
        : "https://www.mercadopago.com.ar/checkout/v1/redirect?pref_id=";
    return baseUrl + paymentId;
}

bool MercadoPago::verifySignature(const std::string& body, const std::string& signature) const {
    if (config_.webhookSecret.empty() || signature.empty()) return true;
    auto expected = crypto::SHA256::hash(body + config_.webhookSecret);
    return expected == signature;
}

nlohmann::json MercadoPago::processWebhook(const nlohmann::json& body,
                                             const std::string& signature) {
    nlohmann::json response;
    response["received"] = true;

    if (!verifySignature(body.dump(), signature)) {
        response["error"] = "Invalid signature";
        response["status"] = "rejected";
        return response;
    }

    std::string action = body.value("action", "");
    std::string type = body.value("type", "");
    std::string paymentId = body["data"].value("id", "");

    response["action"] = action;
    response["type"] = type;
    response["payment_id"] = paymentId;

    if (action == "payment.created" || action == "payment.updated") {
        auto payment = getPayment(paymentId);
        response["payment"] = payment;
        response["status"] = "processed";
    } else if (action == "payment.refunded") {
        response["status"] = "refund_processed";
    } else {
        response["status"] = "acknowledged";
    }

    // Store webhook event
    nlohmann::json event = body;
    event["_processed_at"] = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    event["_status"] = response["status"];
    paymentHistory_.push_back(event);

    return response;
}

nlohmann::json MercadoPago::createPayment(const nlohmann::json& paymentData) {
    std::string paymentId = std::to_string(nextPaymentId_++);

    auto now = std::chrono::system_clock::now();
    auto nowT = std::chrono::system_clock::to_time_t(now);

    std::tm nowTm;
    localtime_s(&nowTm, &nowT);
    std::ostringstream oss;
    oss << std::put_time(&nowTm, "%Y-%m-%dT%H:%M:%S.000-03:00");

    double amount = paymentData.value("amount", 0.0);
    std::string description = paymentData.value("description", "FreeBuff Payment");
    std::string email = paymentData.value("email", "buyer@example.com");
    std::string paymentMethod = paymentData.value("payment_method", "credit_card");

    nlohmann::json payment;
    payment["id"] = paymentId;
    payment["status"] = "pending";
    payment["status_detail"] = "pending_waiting_payment";
    payment["amount"] = amount;
    payment["description"] = description;
    payment["email"] = email;
    payment["payment_method"] = paymentMethod;
    payment["date_created"] = oss.str();
    payment["init_point"] = generatePaymentLink(paymentId);
    payment["sandbox"] = config_.sandboxMode;
    payment["notification_url"] = "https://api.freebuff.com/api/payment/webhook";

    paymentHistory_.push_back(payment);

    return payment;
}

nlohmann::json MercadoPago::getPayment(const std::string& paymentId) {
    for (const auto& p : paymentHistory_) {
        if (p.contains("id") && p["id"].get<std::string>() == paymentId) {
            return p;
        }
    }

    // Return simulated payment for non-existent IDs
    nlohmann::json payment;
    payment["id"] = paymentId;
    payment["status"] = "approved";
    payment["status_detail"] = "accredited";
    payment["amount"] = 100.0;
    payment["description"] = "Simulated Mercado Pago payment";
    return payment;
}

nlohmann::json MercadoPago::cancelPayment(const std::string& paymentId) {
    for (auto& p : paymentHistory_) {
        if (p.contains("id") && p["id"].get<std::string>() == paymentId) {
            p["status"] = "cancelled";
            p["status_detail"] = "cancelled_by_user";
            return p;
        }
    }

    nlohmann::json response;
    response["id"] = paymentId;
    response["status"] = "cancelled";
    response["error"] = "Payment not found, returning simulated response";
    return response;
}

nlohmann::json MercadoPago::refundPayment(const std::string& paymentId, double amount) {
    for (auto& p : paymentHistory_) {
        if (p.contains("id") && p["id"].get<std::string>() == paymentId) {
            p["status"] = "refunded";
            p["status_detail"] = amount > 0 ? "partially_refunded" : "refunded";
            p["refund_amount"] = amount > 0 ? amount : p.value("amount", 0.0);
            return p;
        }
    }

    nlohmann::json response;
    response["id"] = paymentId;
    response["status"] = "refunded";
    response["refund_amount"] = amount;
    return response;
}

} // namespace freebuff::backend
