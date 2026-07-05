#include "mysql_wrapper/MySQLWrapper.h"
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>

namespace freebuff::db {

namespace {
class MySQLLibraryGuard {
public:
    MySQLLibraryGuard() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw MySQLException("WSAStartup failed");
        }
#endif
        mysql_library_init(0, nullptr, nullptr);
    }
    ~MySQLLibraryGuard() {
        mysql_library_end();
#ifdef _WIN32
        WSACleanup();
#endif
    }
    MySQLLibraryGuard(const MySQLLibraryGuard&) = delete;
    MySQLLibraryGuard& operator=(const MySQLLibraryGuard&) = delete;
};
} // anonymous namespace

MySQLWrapper::MySQLWrapper(MySQLConfig config) : config_(std::move(config)) {
    static MySQLLibraryGuard guard;
    (void)guard;
    connect();
}

MySQLWrapper::~MySQLWrapper() { disconnect(); }

MySQLWrapper::MySQLWrapper(MySQLWrapper&& other) noexcept
    : config_(std::move(other.config_)), handle_(other.handle_) {
    other.handle_ = nullptr;
}

MySQLWrapper& MySQLWrapper::operator=(MySQLWrapper&& other) noexcept {
    if (this != &other) {
        disconnect();
        config_ = std::move(other.config_);
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

void MySQLWrapper::connect() {
    if (handle_) { mysql_close(handle_); handle_ = nullptr; }
    handle_ = mysql_init(nullptr);
    if (!handle_) throw MySQLException("mysql_init failed");

    if (config_.connectTimeoutSec > 0) {
        unsigned int t = config_.connectTimeoutSec;
        mysql_options(handle_, MYSQL_OPT_CONNECT_TIMEOUT, &t);
    }
    if (config_.readTimeoutSec > 0) {
        unsigned int t = config_.readTimeoutSec;
        mysql_options(handle_, MYSQL_OPT_READ_TIMEOUT, &t);
    }
    mysql_options(handle_, MYSQL_SET_CHARSET_NAME, config_.charset.c_str());

    MYSQL* ret = mysql_real_connect(handle_, config_.host.c_str(),
        config_.user.c_str(),
        config_.password.empty() ? nullptr : config_.password.c_str(),
        nullptr, config_.port, nullptr, 0);

    if (!ret) {
        std::string err = error();
        mysql_close(handle_); handle_ = nullptr;
        throw MySQLException("mysql_real_connect failed: " + err);
    }

    if (!config_.database.empty()) {
        createDatabaseIfNotExists();
        if (mysql_select_db(handle_, config_.database.c_str()) != 0) {
            std::string err = error();
            disconnect();
            throw MySQLException("mysql_select_db failed: " + err);
        }
    }
}

void MySQLWrapper::disconnect() {
    if (handle_) { mysql_close(handle_); handle_ = nullptr; }
}

bool MySQLWrapper::ping() {
    return handle_ ? mysql_ping(handle_) == 0 : false;
}

MySQLResult MySQLWrapper::query(const std::string& sql) {
    if (!handle_) throw MySQLException("Not connected");
    if (mysql_query(handle_, sql.c_str()) != 0)
        throw MySQLException(std::string("mysql_query: ") + error());

    MYSQL_RES* result = mysql_store_result(handle_);
    MySQLResult out;

    if (result) {
        unsigned int numFields = mysql_num_fields(result);
        MYSQL_FIELD* fields = mysql_fetch_fields(result);
        out.columns.reserve(numFields);
        for (unsigned int i = 0; i < numFields; ++i)
            out.columns.emplace_back(fields[i].name);

        while (MYSQL_ROW row = mysql_fetch_row(result)) {
            unsigned long* lengths = mysql_fetch_lengths(result);
            MySQLRow rowData;
            rowData.reserve(numFields);
            for (unsigned int i = 0; i < numFields; ++i) {
                if (row[i]) rowData.emplace_back(row[i], lengths[i]);
                else rowData.emplace_back("NULL");
            }
            out.rows.push_back(std::move(rowData));
        }
        mysql_free_result(result);
    } else if (mysql_field_count(handle_) == 0) {
        out.affectedRows = mysql_affected_rows(handle_);
        out.insertId = mysql_insert_id(handle_);
    }
    return out;
}

uint64_t MySQLWrapper::execute(const std::string& sql) {
    return query(sql).affectedRows;
}

std::string MySQLWrapper::escape(const std::string& raw) {
    if (!handle_) return raw;
    std::vector<char> buf(raw.size() * 2 + 1);
    unsigned long len = mysql_real_escape_string(handle_, buf.data(), raw.data(), raw.size());
    return std::string(buf.data(), len);
}

void MySQLWrapper::createDatabaseIfNotExists() {
    if (!handle_ || config_.database.empty()) return;
    std::string sql = "CREATE DATABASE IF NOT EXISTS `" + config_.database +
                      "` CHARACTER SET " + config_.charset;
    if (mysql_query(handle_, sql.c_str()) != 0) {
        throw MySQLException("Failed to create database: " + std::string(error()));
    }
}

void MySQLWrapper::createTablesIfNotExists() {
    if (!handle_) return;
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS transactions (
            id              VARCHAR(64) PRIMARY KEY,
            payment_method  VARCHAR(32) NOT NULL,
            amount_cents    BIGINT NOT NULL,
            currency        CHAR(3) DEFAULT 'USD',
            status          VARCHAR(16) NOT NULL DEFAULT 'PENDING',
            description     TEXT,
            created_at      DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
            updated_at      DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3) ON UPDATE CURRENT_TIMESTAMP(3),
            INDEX idx_created (created_at),
            INDEX idx_status (status)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
        CREATE TABLE IF NOT EXISTS payment_methods (
            id           INT AUTO_INCREMENT PRIMARY KEY,
            name         VARCHAR(64) NOT NULL UNIQUE,
            display_name VARCHAR(128) NOT NULL,
            enabled      TINYINT(1) NOT NULL DEFAULT 1,
            config_json  TEXT,
            created_at   DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
        CREATE TABLE IF NOT EXISTS logs (
            id         BIGINT AUTO_INCREMENT PRIMARY KEY,
            level      VARCHAR(8) NOT NULL,
            message    TEXT NOT NULL,
            created_at DATETIME(3) NOT NULL DEFAULT CURRENT_TIMESTAMP(3),
            INDEX idx_level (level),
            INDEX idx_created (created_at)
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    )SQL";
    // execute each statement separately
    std::istringstream stream(sql);
    std::string stmt;
    while (std::getline(stream, stmt, ';')) {
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\n\r"));
            s.erase(s.find_last_not_of(" \t\n\r") + 1);
        };
        trim(stmt);
        if (stmt.empty() || stmt.rfind("--", 0) == 0) continue;
        stmt += ';';
        if (mysql_query(handle_, stmt.c_str()) != 0) {
            // Tables may already exist; non-fatal
            continue;
        }
    }
}

void MySQLWrapper::beginTransaction() { execute("START TRANSACTION"); }
void MySQLWrapper::commit() { execute("COMMIT"); }
void MySQLWrapper::rollback() { execute("ROLLBACK"); }

const char* MySQLWrapper::error() const noexcept {
    return handle_ ? mysql_error(handle_) : "No connection";
}

} // namespace freebuff::db
