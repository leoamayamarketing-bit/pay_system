#ifndef FREEBUFF_DASHBOARDSTATS_H
#define FREEBUFF_DASHBOARDSTATS_H

#include <nlohmann/json.hpp>
#include "persistence/ITransactionRepository.h"

namespace freebuff::backend {

class DashboardStats {
public:
    explicit DashboardStats(const persist::ITransactionRepository& repo);

    nlohmann::json getStats() const;
    nlohmann::json getRevenueByDay(int days = 7) const;
    nlohmann::json getTransactionsByMethod() const;
    nlohmann::json getTransactionsByStatus() const;
    nlohmann::json getRecentTransactions(int limit = 10) const;

private:
    const persist::ITransactionRepository& repo_;
};

} // namespace freebuff::backend
#endif
