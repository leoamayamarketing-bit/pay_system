#include "backend/DashboardStats.h"
#include <algorithm>
#include <map>

namespace freebuff::backend {

DashboardStats::DashboardStats(const persist::ITransactionRepository& repo)
    : repo_(repo) {}

nlohmann::json DashboardStats::getStats() const {
    auto transactions = repo_.findAll();

    int64_t totalVolume = 0;
    int successCount = 0;
    int failedCount = 0;
    int refundedCount = 0;
    std::map<std::string, int> methodCount;

    for (const auto& tx : transactions) {
        totalVolume += tx.amount.cents();
        if (tx.status == freebuff::PaymentStatus::Success) ++successCount;
        else if (tx.status == freebuff::PaymentStatus::Refunded) ++refundedCount;
        else ++failedCount;
        methodCount[tx.paymentMethod]++;
    }

    nlohmann::json methodsJson = nlohmann::json::object();
    for (const auto& [method, count] : methodCount) {
        methodsJson[method] = count;
    }

    auto now = std::chrono::system_clock::now();
    auto nowT = std::chrono::system_clock::to_time_t(now);
    std::tm nowTm;
#ifdef _WIN32
    localtime_s(&nowTm, &nowT);
#else
    localtime_r(&nowT, &nowTm);
#endif

    nlohmann::json stats;
    stats["total_transactions"] = transactions.size();
    stats["total_volume_cents"] = totalVolume;
    stats["total_volume_dollars"] = static_cast<double>(totalVolume) / 100.0;
    stats["successful"] = successCount;
    stats["failed"] = failedCount;
    stats["refunded"] = refundedCount;
    stats["success_rate"] = transactions.empty() ? 0.0 :
        (static_cast<double>(successCount) / transactions.size()) * 100.0;
    stats["by_method"] = methodsJson;
    std::ostringstream ts;
    ts << std::put_time(&nowTm, "%Y-%m-%dT%H:%M:%SZ");
    stats["generated_at"] = ts.str();

    return stats;
}

nlohmann::json DashboardStats::getRevenueByDay(int days) const {
    auto transactions = repo_.findAll();

    auto now = std::chrono::system_clock::now();
    std::map<std::string, int64_t> dailyRevenue;

    for (const auto& tx : transactions) {
        if (tx.status != freebuff::PaymentStatus::Success) continue;

        auto t = std::chrono::system_clock::to_time_t(tx.timestamp);
        std::tm tm;
        localtime_s(&tm, &t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d");
        dailyRevenue[oss.str()] += tx.amount.cents();
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [date, revenue] : dailyRevenue) {
        nlohmann::json day;
        day["date"] = date;
        day["revenue_cents"] = revenue;
        day["revenue_dollars"] = static_cast<double>(revenue) / 100.0;
        result.push_back(day);
    }

    return result;
}

nlohmann::json DashboardStats::getTransactionsByMethod() const {
    auto transactions = repo_.findAll();
    std::map<std::string, int> methodCount;

    for (const auto& tx : transactions) {
        methodCount[tx.paymentMethod]++;
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [method, count] : methodCount) {
        nlohmann::json entry;
        entry["method"] = method;
        entry["count"] = count;
        result.push_back(entry);
    }

    return result;
}

nlohmann::json DashboardStats::getTransactionsByStatus() const {
    auto transactions = repo_.findAll();
    std::map<std::string, int> statusCount;

    for (const auto& tx : transactions) {
        statusCount[freebuff::paymentStatusToString(tx.status)]++;
    }

    nlohmann::json result = nlohmann::json::object();
    for (const auto& [status, count] : statusCount) {
        result[status] = count;
    }

    return result;
}

nlohmann::json DashboardStats::getRecentTransactions(int limit) const {
    auto transactions = repo_.findAll();
    std::sort(transactions.begin(), transactions.end(),
        [](const auto& a, const auto& b) {
            return a.timestamp > b.timestamp;
        });

    if (static_cast<size_t>(limit) < transactions.size()) {
        transactions.resize(limit);
    }

    nlohmann::json result = nlohmann::json::array();
    for (const auto& tx : transactions) {
        auto t = std::chrono::system_clock::to_time_t(tx.timestamp);
        std::tm tm;
        localtime_s(&tm, &t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");

        nlohmann::json entry;
        entry["id"] = tx.id;
        entry["payment_method"] = tx.paymentMethod;
        entry["amount_cents"] = tx.amount.cents();
        entry["amount_dollars"] = tx.amount.toDollars();
        entry["status"] = freebuff::paymentStatusToString(tx.status);
        entry["description"] = tx.description;
        entry["timestamp"] = oss.str();
        result.push_back(entry);
    }

    return result;
}

} // namespace freebuff::backend
