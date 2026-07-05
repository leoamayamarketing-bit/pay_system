#ifndef FREEBUFF_SQLITEREPOSITORY_H
#define FREEBUFF_SQLITEREPOSITORY_H

#include "ITransactionRepository.h"
#include "ILogger.h"
#include <string>
#include <memory>
#include <mutex>

struct sqlite3;
struct sqlite3_stmt;

namespace freebuff::persist {

class SQLiteRepository : public ITransactionRepository {
public:
    explicit SQLiteRepository(const std::string& dbPath = "freebuff.db",
                              std::unique_ptr<ILogger> logger = nullptr);
    ~SQLiteRepository() override;

    bool save(const Transaction& transaction) override;
    std::optional<Transaction> findById(const std::string& id) const override;
    std::vector<Transaction> findAll() const override;
    std::vector<Transaction> findByDateRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const override;
    bool remove(const std::string& id) override;
    size_t count() const noexcept override;
    bool clear() override;

    bool isOpen() const noexcept { return db_ != nullptr; }
    bool execRaw(const std::string& sql);

private:
    sqlite3* db_{nullptr};
    std::string dbPath_;
    mutable std::mutex mutex_;
    std::unique_ptr<ILogger> logger_;

    void ensureSchema();
    bool bindTransaction(sqlite3_stmt* stmt, const Transaction& tx);
    Transaction rowToTransaction(sqlite3_stmt* stmt) const;
    void log(LogLevel level, const std::string& msg) const;
};

} // namespace freebuff::persist
#endif
