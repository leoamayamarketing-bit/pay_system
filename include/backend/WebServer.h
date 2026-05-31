#ifndef FREEBUFF_WEBSERVER_H
#define FREEBUFF_WEBSERVER_H

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <iostream>
#include <nlohmann/json.hpp>

#include "backend/HttpServer.h"
#include "backend/AuthMiddleware.h"
#include "auth/AuthService.h"
#include "persistence/ITransactionRepository.h"
#include "hardware/HardwareAbstraction.h"
#include "backend/MercadoPago.h"
#include "backend/DashboardStats.h"

namespace freebuff { class PaymentFactory; }

namespace freebuff::backend {

class WebServer {
public:
    WebServer(uint16_t port = 8080);
    ~WebServer();

    void start();
    void stop();
    bool isRunning() const { return running_; }
    uint16_t port() const { return port_; }

    void setTransactionRepo(freebuff::persist::ITransactionRepository* repo) { txRepo_ = repo; }
    void setHardware(freebuff::hw::HardwareAbstraction* hw) { hardware_ = hw; }

    static std::string generateId();
    static nlohmann::json transactionToJson(const freebuff::Transaction& tx);

private:
    uint16_t port_;
    std::atomic<bool> running_{false};
    std::unique_ptr<HttpServer> httpServer_;

    auth::AuthService authService_;
    MercadoPago mercadoPago_;

    freebuff::persist::ITransactionRepository* txRepo_{nullptr};
    freebuff::hw::HardwareAbstraction* hardware_{nullptr};

    void setupRoutes();
    HttpResponse handleCors(HttpResponse res);

    // Auth helpers
    std::optional<HttpResponse> requireAuth(const HttpRequest& req);
    std::optional<HttpResponse> requireAdmin(const HttpRequest& req);
    AuthMiddleware authMiddleware_{authService_};
};

} // namespace freebuff::backend
#endif // FREEBUFF_WEBSERVER_H
