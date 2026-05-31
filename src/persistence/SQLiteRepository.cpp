#include "persistence/SQLiteRepository.h"
#include <sqlite3.h>
#include <sstream>
#include <algorithm>

namespace freebuff::persist {

SQLiteRepository::SQLiteRepository(const std::string& dbPath,
                                    std::unique_ptr<ILogger> logger)
    : dbPath_(dbPath), logger_(std::move(logger))
{
    if (!logger_) {
        logger_ = createConsoleLogger(LogLevel::Warning);
    }

    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        log(LogLevel::Error, "Failed to open SQLite DB: " + std::string(sqlite3_errmsg(db_)));
        if (db_) { sqlite3_close(db_); db_ = nullptr; }
        return;
    }

    // Enable WAL mode for better concurrency
    execRaw("PRAGMA journal_mode=WAL");
    execRaw("PRAGMA synchronous=NORMAL");

    ensureSchema();
    log(LogLevel::Info, "SQLite repository opened: " + dbPath_);
}

SQLiteRepository::~SQLiteRepository() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
        log(LogLevel::Info, "SQLite repository closed");
    }
}

void SQLiteRepository::log(LogLevel level, const std::string& msg) const {
    if (logger_) logger_->log(level, "[SQLiteRepo] " + msg);
}

void SQLiteRepository::ensureSchema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id            TEXT PRIMARY KEY,
            payment_method TEXT NOT NULL,
            amount_cents  INTEGER NOT NULL,
            status        INTEGER NOT NULL DEFAULT 0,
            description   TEXT NOT NULL DEFAULT '',
            timestamp_sec INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_transactions_timestamp
            ON transactions(timestamp_sec);
        CREATE INDEX IF NOT EXISTS idx_transactions_status
            ON transactions(status);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        log(LogLevel::Error, "Schema creation failed: " + std::string(errMsg));
        sqlite3_free(errMsg);
    }
}

bool SQLiteRepository::execRaw(const std::string& sql) {
    if (!db_) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        log(LogLevel::Error, "execRaw failed: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool SQLiteRepository::bindTransaction(sqlite3_stmt* stmt, const Transaction& tx) {
    sqlite3_bind_text(stmt, 1, tx.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, tx.paymentMethod.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, tx.amount.cents());
    sqlite3_bind_int(stmt, 4, static_cast<int>(tx.status));
    sqlite3_bind_text(stmt, 5, tx.description.c_str(), -1, SQLITE_TRANSIENT);
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        tx.timestamp.time_since_epoch()).count();
    sqlite3_bind_int64(stmt, 6, ts);
    return true;
}

Transaction SQLiteRepository::rowToTransaction(sqlite3_stmt* stmt) const {
    Transaction tx;
    tx.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    tx.paymentMethod = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    tx.amount = Money::fromCents(sqlite3_column_int64(stmt, 2));
    tx.status = static_cast<PaymentStatus>(sqlite3_column_int(stmt, 3));
    tx.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    auto ts = sqlite3_column_int64(stmt, 5);
    tx.timestamp = std::chrono::system_clock::time_point(
        std::chrono::seconds(ts));
    return tx;
}

bool SQLiteRepository::save(const Transaction& transaction) {
    if (!db_) return false;
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = R"(
        INSERT OR REPLACE INTO transactions
            (id, payment_method, amount_cents, status, description, timestamp_sec)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        log(LogLevel::Error, "save prepare failed: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    bindTransaction(stmt, transaction);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        log(LogLevel::Error, "save step failed: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    return true;
}

std::optional<Transaction> SQLiteRepository::findById(const std::string& id) const {
    if (!db_) return std::nullopt;
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, payment_method, amount_cents, status, description, timestamp_sec "
                      "FROM transactions WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return std::nullopt;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<Transaction> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = rowToTransaction(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Transaction> SQLiteRepository::findAll() const {
    if (!db_) return {};
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, payment_method, amount_cents, status, description, timestamp_sec "
                      "FROM transactions ORDER BY timestamp_sec DESC";

    sqlite3_stmt* stmt = nullptr;
    std::vector<Transaction> results;

    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(rowToTransaction(stmt));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<Transaction> SQLiteRepository::findByDateRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    if (!db_) return {};
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, payment_method, amount_cents, status, description, timestamp_sec "
                      "FROM transactions WHERE timestamp_sec >= ? AND timestamp_sec <= ? "
                      "ORDER BY timestamp_sec DESC";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    auto startSec = std::chrono::duration_cast<std::chrono::seconds>(
        start.time_since_epoch()).count();
    auto endSec = std::chrono::duration_cast<std::chrono::seconds>(
        end.time_since_epoch()).count();
    sqlite3_bind_int64(stmt, 1, startSec);
    sqlite3_bind_int64(stmt, 2, endSec);

    std::vector<Transaction> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(rowToTransaction(stmt));
    }
    sqlite3_finalize(stmt);
    return results;
}

bool SQLiteRepository::remove(const std::string& id) {
    if (!db_) return false;
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM transactions WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

size_t SQLiteRepository::count() const noexcept {
    if (!db_) return 0;
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        const char* sql = "SELECT COUNT(*) FROM transactions";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
        size_t cnt = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cnt = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
        }
        sqlite3_finalize(stmt);
        return cnt;
    } catch (...) {
        return 0;
    }
}

bool SQLiteRepository::clear() {
    if (!db_) return false;
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM transactions";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        log(LogLevel::Error, "clear failed: " + std::string(errMsg));
        sqlite3_free(errMsg);
        return false;
    }

    log(LogLevel::Info, "All transactions cleared");
    return true;
}

} // namespace freebuff::persist
