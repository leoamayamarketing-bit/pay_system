#ifndef FREEBUFF_ITRANSACTIONREPOSITORY_H
#define FREEBUFF_ITRANSACTIONREPOSITORY_H

#include <vector>
#include <optional>
#include "core/Transaction.h"

namespace freebuff::persist {

class ITransactionRepository {
public:
    virtual ~ITransactionRepository() = default;

    virtual bool save(const Transaction& transaction) = 0;
    virtual std::optional<Transaction> findById(const std::string& id) const = 0;
    virtual std::vector<Transaction> findAll() const = 0;
    virtual std::vector<Transaction> findByDateRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const = 0;
    virtual bool remove(const std::string& id) = 0;
    virtual size_t count() const noexcept = 0;
    virtual bool clear() = 0;
};

} // namespace freebuff::persist

#endif
