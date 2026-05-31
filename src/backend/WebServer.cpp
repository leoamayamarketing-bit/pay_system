#include "backend/WebServer.h"
#include "core/IPaymentMethod.h"
#include "payment/CreditCard.h"
#include "payment/DebitCard.h"
#include "payment/QRPayment.h"
#include "payment/NFCPayment.h"
#include "payment/PaymentFactory.h"
#include "qr_sim/QRSimulator.h"
#include "hardware/IUSBDevice.h"

#include <sstream>
#include <random>
#include <iomanip>
#include <algorithm>

namespace freebuff::backend {

WebServer::WebServer(uint16_t port) : port_(port) {}
WebServer::~WebServer() { stop(); }

std::string WebServer::generateId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) ss << std::hex << dis(gen);
    return ss.str();
}

nlohmann::json WebServer::transactionToJson(const freebuff::Transaction& tx) {
    auto t = std::chrono::system_clock::to_time_t(tx.timestamp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");

    nlohmann::json j;
    j["id"] = tx.id;
    j["payment_method"] = tx.paymentMethod;
    j["amount_cents"] = tx.amount.cents();
    j["amount"] = tx.amount.toDollars();
    j["currency"] = "USD";
    j["status"] = paymentStatusToString(tx.status);
    j["description"] = tx.description;
    j["timestamp"] = ss.str();
    return j;
}

HttpResponse WebServer::handleCors(HttpResponse res) {
    res.extraHeaders["Access-Control-Allow-Origin"] = "*";
    res.extraHeaders["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    res.extraHeaders["Access-Control-Allow-Headers"] = "Content-Type, Authorization";
    return res;
}

std::optional<HttpResponse> WebServer::requireAuth(const HttpRequest& req) {
    return authMiddleware_.requireAuth(req);
}

std::optional<HttpResponse> WebServer::requireAdmin(const HttpRequest& req) {
    return authMiddleware_.requireRole(req, "admin");
}

void WebServer::setupRoutes() {
    using nlohmann::json;

    // GET /api/health (public)
    httpServer_->get("/api/health", [this](const HttpRequest&) -> HttpResponse {
        HttpResponse res;
        json j;
        j["status"] = "ok";
        j["server"] = "FreeBuff Payment System";
        j["version"] = "2.0.0";
        j["timestamp"] = std::to_string(std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now()));
        j["database"] = txRepo_ ? "connected" : "not configured";
        res.body = j.dump();
        return handleCors(res);
    });

    // GET /api/payment/methods (auth required)
    httpServer_->get("/api/payment/methods", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        auto& factory = freebuff::PaymentFactory::instance();
        auto methods = factory.availableMethods();
        json arr = json::array();
        for (const auto& m : methods) {
            auto method = factory.create(m);
            json item;
            item["name"] = m;
            item["description"] = method ? method->description() : "";
            item["type"] = m;
            arr.push_back(item);
        }
        res.body = arr.dump();
        return handleCors(res);
    });

    // POST /api/payment/create (auth required)
    httpServer_->post("/api/payment/create", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            std::string method_name = body.value("method", "");
            double amount_val = body.value("amount", 0.0);

            if (method_name.empty() || amount_val <= 0) {
                res.statusCode = 400;
                json err;
                err["status"] = "error";
                err["message"] = "Invalid request: 'method' and 'amount' are required";
                res.body = err.dump();
                return handleCors(res);
            }

            auto& factory = freebuff::PaymentFactory::instance();
            auto payment = factory.create(method_name);
            if (!payment) {
                res.statusCode = 400;
                json err;
                err["status"] = "error";
                err["message"] = "Unknown payment method: " + method_name;
                res.body = err.dump();
                return handleCors(res);
            }

            auto validation = payment->validate();
            if (!validation.isSuccess()) {
                res.statusCode = 400;
                json err;
                err["status"] = "error";
                err["message"] = validation.message;
                res.body = err.dump();
                return handleCors(res);
            }

            Money money = Money::fromDollars(amount_val);
            auto result = payment->process(money);

            json j;
            j["status"] = result.isSuccess() ? "success" : "error";
            j["message"] = result.message;
            if (result.transactionId) j["transaction_id"] = *result.transactionId;
            j["amount"] = amount_val;
            j["currency"] = "USD";
            j["method"] = method_name;

            if (result.isSuccess() && result.transactionId && txRepo_) {
                Transaction tx{*result.transactionId, method_name, money,
                    PaymentStatus::Success, payment->description(),
                    std::chrono::system_clock::now()};
                txRepo_->save(tx);
            }
            res.statusCode = result.isSuccess() ? 200 : 400;
            res.body = j.dump();
            return handleCors(res);
        } catch (const std::exception& e) {
            res.statusCode = 400;
            json err;
            err["status"] = "error";
            err["message"] = std::string("Parse error: ") + e.what();
            res.body = err.dump();
            return handleCors(res);
        }
    });

    // GET /api/payment/status/{id} (auth required)
    httpServer_->get("/api/payment/status/:id", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        auto it = req.pathParams.find("id");
        std::string id = (it != req.pathParams.end()) ? it->second : "";
        if (id.empty()) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = "Transaction ID is required";
            res.body = err.dump(); return handleCors(res);
        }
        if (!txRepo_) {
            res.statusCode = 500; json err;
            err["status"] = "error"; err["message"] = "Repository not configured";
            res.body = err.dump(); return handleCors(res);
        }
        auto tx = txRepo_->findById(id);
        if (!tx) {
            res.statusCode = 404; json err;
            err["status"] = "error"; err["message"] = "Transaction not found: " + id;
            res.body = err.dump(); return handleCors(res);
        }
        res.body = transactionToJson(*tx).dump();
        return handleCors(res);
    });

    // POST /api/payment/webhook (auth required)
    httpServer_->post("/api/payment/webhook", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            auto sigIt = req.headers.find("X-Signature");
            std::string sig = (sigIt != req.headers.end()) ? sigIt->second : "";
            json result = mercadoPago_.processWebhook(body, sig);
            res.body = result.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("Webhook error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });

    // GET /api/qr/generate (auth required)
    httpServer_->get("/api/qr/generate", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        try {
            auto& qp = req.queryParams;
            std::string merchant = qp.count("merchant") ? qp.at("merchant") : "MERCH001";
            std::string reference = qp.count("reference") ? qp.at("reference") : "QR-" + generateId();
            double amount_val = qp.count("amount") ? std::stod(qp.at("amount")) : 0.0;

            qr::QRPayload payload;
            payload.merchantId = merchant;
            payload.merchantName = "FreeBuff Store";
            payload.reference = reference;
            payload.amount = Money::fromDollars(amount_val);

            std::vector<std::string> qrCode = qr::QRSimulator::generateFromPayload(payload);

            json j;
            j["qr_code_lines"] = qrCode;
            j["payload"] = {
                {"merchant", merchant},
                {"reference", reference},
                {"amount", amount_val}
            };
            res.body = j.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("QR error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });

    // GET /api/transactions (auth required)
    httpServer_->get("/api/transactions", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        if (!txRepo_) {
            res.statusCode = 500; json err;
            err["status"] = "error"; err["message"] = "Repository not configured";
            res.body = err.dump(); return handleCors(res);
        }
        int limit = 50;
        auto limitIt = req.queryParams.find("limit");
        if (limitIt != req.queryParams.end()) {
            limit = std::stoi(limitIt->second);
        }
        auto txs = txRepo_->findAll();
        json arr = json::array();
        int count = 0;
        for (const auto& tx : txs) {
            if (count >= limit) break;
            arr.push_back(transactionToJson(tx));
            ++count;
        }
        json j;
        j["transactions"] = arr;
        j["count"] = arr.size();
        res.body = j.dump();
        return handleCors(res);
    });

    // GET /api/dashboard/stats (auth required)
    httpServer_->get("/api/dashboard/stats", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        if (!txRepo_) {
            res.statusCode = 500; json err;
            err["status"] = "error"; err["message"] = "Repository not configured";
            res.body = err.dump(); return handleCors(res);
        }
        DashboardStats stats(*txRepo_);
        json result = stats.getStats();
        result["status"] = "ok";
        res.body = result.dump();
        return handleCors(res);
    });

    // GET /api/dashboard/revenue (auth required)
    httpServer_->get("/api/dashboard/revenue", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        if (!txRepo_) {
            res.statusCode = 500; json err;
            err["status"] = "error"; err["message"] = "Repository not configured";
            res.body = err.dump(); return handleCors(res);
        }
        int days = 7;
        auto daysIt = req.queryParams.find("days");
        if (daysIt != req.queryParams.end()) {
            days = std::stoi(daysIt->second);
        }
        DashboardStats stats(*txRepo_);
        json result = stats.getRevenueByDay(days);
        result["status"] = "ok";
        res.body = result.dump();
        return handleCors(res);
    });

    // POST /api/auth/login (public)
    httpServer_->post("/api/auth/login", [this](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            auto authRes = authService_.login(body.value("username", ""), body.value("password", ""));
            json j;
            j["success"] = authRes.success;
            if (authRes.success) {
                j["token"] = authRes.token;
                j["user_id"] = authRes.userId;
                j["username"] = authRes.username;
                j["role"] = authRes.role;
                j["expires_at"] = authRes.expiresAt;
            } else {
                j["error"] = authRes.error;
                res.statusCode = 401;
            }
            res.body = j.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("Auth error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });

    // POST /api/auth/register (public)
    httpServer_->post("/api/auth/register", [this](const HttpRequest& req) -> HttpResponse {
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            auto authRes = authService_.registerUser(
                body.value("username", ""), body.value("email", ""),
                body.value("password", ""), body.value("role", "merchant"));
            json j;
            j["success"] = authRes.success;
            if (authRes.success) {
                j["user_id"] = authRes.userId;
                j["username"] = authRes.username;
                j["token"] = authRes.token;
            } else {
                j["error"] = authRes.error;
                res.statusCode = 409;
            }
            res.body = j.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("Register error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });

    // POST /api/mercadopago/webhook (auth required)
    httpServer_->post("/api/mercadopago/webhook", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            auto sigIt = req.headers.find("X-Signature");
            std::string sig = (sigIt != req.headers.end()) ? sigIt->second : "";
            json result = mercadoPago_.processWebhook(body, sig);
            res.body = result.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("Webhook error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });

    // GET /api/devices (auth required)
    httpServer_->get("/api/devices", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        json arr = json::array();
        if (hardware_ && hardware_->isInitialized()) {
            auto devices = hardware_->usbManager().listDevices();
            for (const auto& d : devices) {
                json item;
                item["bus"] = d.busNumber;
                item["device"] = d.deviceNumber;
                item["vendor_id"] = d.vendorId;
                item["product_id"] = d.productId;
                item["vendor"] = d.vendorName;
                item["product"] = d.productName;
                item["driver"] = d.driver;
                arr.push_back(item);
            }
        }
        json j;
        j["devices"] = arr;
        j["count"] = arr.size();
        res.body = j.dump();
        return handleCors(res);
    });

    // GET /api/hardware/status (auth required)
    httpServer_->get("/api/hardware/status", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAuth(req)) return *err;
        HttpResponse res;
        json j;
        if (hardware_) {
            j["initialized"] = hardware_->isInitialized();
            j["nfc_connected"] = true;
            j["report"] = hardware_->hardwareReport();
        } else {
            j["initialized"] = false;
            j["report"] = "Hardware not configured";
        }
        res.body = j.dump();
        return handleCors(res);
    });

    // POST /api/hardware/nfc/simulate (admin required)
    httpServer_->post("/api/hardware/nfc/simulate", [this](const HttpRequest& req) -> HttpResponse {
        if (auto err = requireAdmin(req)) return *err;
        HttpResponse res;
        try {
            auto body = json::parse(req.body);
            if (!hardware_) {
                res.statusCode = 500; json err;
                err["status"] = "error"; err["message"] = "Hardware not configured";
                res.body = err.dump(); return handleCors(res);
            }
            bool cardPresent = body.value("card_present", true);
            bool nfcConnected = body.value("nfc_connected", true);
            hardware_->simulateCardPresent(cardPresent);
            hardware_->simulateNfcConnection(nfcConnected);
            json j;
            j["success"] = true;
            j["card_present"] = cardPresent;
            j["nfc_connected"] = nfcConnected;
            res.body = j.dump();
        } catch (const std::exception& e) {
            res.statusCode = 400; json err;
            err["status"] = "error"; err["message"] = std::string("NFC error: ") + e.what();
            res.body = err.dump();
        }
        return handleCors(res);
    });
}

void WebServer::start() {
    if (running_) return;
    if (!httpServer_) {
        httpServer_ = std::make_unique<HttpServer>(port_);
    }
    setupRoutes();
    if (httpServer_->start()) {
        running_ = true;
        std::cout << "[WebServer] Started on http://0.0.0.0:" << port_ << std::endl;
        std::cout << "[WebServer] API endpoints:" << std::endl;
        std::cout << "  GET  /api/health" << std::endl;
        std::cout << "  GET  /api/payment/methods" << std::endl;
        std::cout << "  POST /api/payment/create" << std::endl;
        std::cout << "  GET  /api/payment/status/:id" << std::endl;
        std::cout << "  POST /api/payment/webhook" << std::endl;
        std::cout << "  GET  /api/qr/generate" << std::endl;
        std::cout << "  GET  /api/transactions" << std::endl;
        std::cout << "  GET  /api/dashboard/stats" << std::endl;
        std::cout << "  GET  /api/dashboard/revenue" << std::endl;
        std::cout << "  POST /api/auth/login" << std::endl;
        std::cout << "  POST /api/auth/register" << std::endl;
        std::cout << "  POST /api/mercadopago/webhook" << std::endl;
        std::cout << "  GET  /api/devices" << std::endl;
        std::cout << "  GET  /api/hardware/status" << std::endl;
        std::cout << "  POST /api/hardware/nfc/simulate" << std::endl;
    } else {
        std::cerr << "[WebServer] Failed to start on port " << port_ << std::endl;
    }
}

void WebServer::stop() {
    running_ = false;
    if (httpServer_) {
        httpServer_->stop();
    }
}

} // namespace freebuff::backend
