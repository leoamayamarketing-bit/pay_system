#ifndef FREEBUFF_MYSQLWRAPPER_H
#define FREEBUFF_MYSQLWRAPPER_H

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #ifndef XAMPP_MYSQL_INCLUDE_DIR
        #define XAMPP_MYSQL_INCLUDE_DIR "C:/xampp/mysql/include"
    #endif
    #ifndef XAMPP_MYSQL_LIB_DIR
        #define XAMPP_MYSQL_LIB_DIR     "C:/xampp/mysql/lib"
    #endif
    #pragma push_macro("_WIN32_WINNT")
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
    #include <mysql.h>
    #pragma comment(lib, "libmysql.lib")
    #pragma pop_macro("_WIN32_WINNT")
#else
    #if __has_include(<mariadb/mysql.h>)
        #include <mariadb/mysql.h>
    #elif __has_include(<mysql/mysql.h>)
        #include <mysql/mysql.h>
    #else
        #include <mysql.h>
    #endif
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <stdexcept>

namespace freebuff::db {

class MySQLException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

using MySQLRow = std::vector<std::string>;

struct MySQLResult {
    std::vector<std::string> columns;
    std::vector<MySQLRow>    rows;
    uint64_t                 affectedRows{0};
    uint64_t                 insertId{0};
    bool empty() const noexcept { return rows.empty(); }
    size_t rowCount() const noexcept { return rows.size(); }
};

struct MySQLConfig {
    std::string host     = "localhost";
    uint16_t    port     = 3306;
    std::string user     = "root";
    std::string password = "";
    std::string database = "freebuff";
    std::string charset  = "utf8mb4";
    unsigned int connectTimeoutSec{5};
    unsigned int readTimeoutSec{5};
};

class MySQLWrapper {
public:
    explicit MySQLWrapper(MySQLConfig config = {});
    ~MySQLWrapper();
    MySQLWrapper(MySQLWrapper&& other) noexcept;
    MySQLWrapper& operator=(MySQLWrapper&& other) noexcept;
    MySQLWrapper(const MySQLWrapper&) = delete;
    MySQLWrapper& operator=(const MySQLWrapper&) = delete;

    void connect();
    void disconnect();
    bool ping();
    bool isConnected() const noexcept { return handle_ != nullptr; }

    MySQLResult query(const std::string& sql);
    uint64_t execute(const std::string& sql);
    std::string escape(const std::string& raw);

    void createDatabaseIfNotExists();
    void createTablesIfNotExists();

    void beginTransaction();
    void commit();
    void rollback();

    const MySQLConfig& config() const noexcept { return config_; }
    const char* error() const noexcept;

private:
    MySQLConfig config_;
    MYSQL*      handle_{nullptr};
    void initLibrary();
    void cleanupLibrary();
};

using DBResult = MySQLResult;
using DBConfig = MySQLConfig;
using DBException = MySQLException;
using DBWrapper = MySQLWrapper;

} // namespace freebuff::db
#endif
