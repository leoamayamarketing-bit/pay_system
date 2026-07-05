# ═══════════════════════════════════════════════════════════════════════════════
#  FreeBuff Embedded Payment System  –  CMake Build Script
#  Targets:  12 static libraries + 1 executable
#  Platforms: Windows (XAMPP/MinGW/MSVC) / Linux
#  Dependencies: nlohmann-json (header-only, fetched)
#  Standard:  C++20
# ═══════════════════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.20)
project(FreeBuffPaymentSystem VERSION 2.0.0 LANGUAGES CXX)

# ── Global settings ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/libs")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/libs")

# ── Options ──────────────────────────────────────────────────────────────────
option(BUILD_TESTS           "Build unit tests"               ON)
option(WITH_MYSQL            "Build MySQL wrapper library"    ON)
option(WITH_NFC_SIM          "Build NFC simulation library"   ON)
option(WITH_QR_SIM           "Build QR simulation library"    ON)
option(WITH_BACKEND          "Build web backend (REST API)"   ON)
option(WITH_AUTH             "Build authentication module"    ON)
option(WITH_CRYPTO           "Build crypto module"            ON)
option(WITH_SQLITE           "Build SQLite persistence"       ON)

# ── Threads ──────────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)

# ── Platform-specific socket library ─────────────────────────────────────────
if(WIN32)
    set(SOCKET_LIBS ws2_32)
else()
    set(SOCKET_LIBS "")
endif()

# ── Fetch nlohmann/json (header-only) ────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

# ── Compiler flags ───────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# ── Include paths ────────────────────────────────────────────────────────────
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
include_directories(${INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/sqlite)

# ═══════════════════════════════════════════════════════════════════════════════
#  Source file lists
# ═══════════════════════════════════════════════════════════════════════════════
set(CORE_SOURCES    src/core/PaymentResult.cpp src/core/Transaction.cpp src/core/Money.cpp)
set(PAYMENT_SOURCES src/payment/CreditCard.cpp src/payment/DebitCard.cpp src/payment/QRPayment.cpp src/payment/NFCPayment.cpp src/payment/PaymentFactory.cpp)
set(HARDWARE_SOURCES src/hardware/HardwareAbstraction.cpp src/hardware/USBDeviceManager.cpp)
set(PERSISTENCE_SOURCES src/persistence/TransactionRepository.cpp src/persistence/Logger.cpp)
set(UI_SOURCES src/ui/TerminalUI.cpp)
set(CRYPTO_SOURCES src/crypto/SHA256.cpp src/crypto/HMAC.cpp)
set(AUTH_SOURCES src/auth/JWT.cpp src/auth/AuthService.cpp)
set(BACKEND_SOURCES src/backend/WebServer.cpp src/backend/HttpServer.cpp src/backend/DashboardStats.cpp src/backend/MercadoPago.cpp)

# ═══════════════════════════════════════════════════════════════════════════════
#  1.  libcore.a
# ═══════════════════════════════════════════════════════════════════════════════
add_library(core STATIC ${CORE_SOURCES})
target_include_directories(core PUBLIC ${INCLUDE_DIR})

#  2.  libqr_sim.a
if(WITH_QR_SIM)
    add_library(qr_sim STATIC src/qr_sim/QRSimulator.cpp)
    target_include_directories(qr_sim PUBLIC ${INCLUDE_DIR})
    target_link_libraries(qr_sim PUBLIC core)
endif()

#  3.  libnfc_sim.a
if(WITH_NFC_SIM)
    add_library(nfc_sim STATIC src/nfc_sim/NFCReader.cpp)
    target_include_directories(nfc_sim PUBLIC ${INCLUDE_DIR})
    target_link_libraries(nfc_sim PUBLIC core)
endif()

#  4.  libpayment.a
add_library(payment STATIC ${PAYMENT_SOURCES})
target_include_directories(payment PUBLIC ${INCLUDE_DIR})
target_link_libraries(payment PUBLIC core)
if(WITH_QR_SIM)
    target_link_libraries(payment PUBLIC qr_sim)
endif()

#  5.  libhardware.a
add_library(hardware STATIC ${HARDWARE_SOURCES})
target_include_directories(hardware PUBLIC ${INCLUDE_DIR})
target_link_libraries(hardware PUBLIC core)
if(WITH_NFC_SIM)
    target_link_libraries(hardware PUBLIC nfc_sim)
endif()

#  6.  libpersistence.a
set(PERSISTENCE_EXTRA_SOURCES)
set(PERSISTENCE_EXTRA_LIBS)
if(WITH_SQLITE)
    # SQLite3 amalgamation as an object library
    add_library(sqlite3 OBJECT sqlite/sqlite3.c)
    target_include_directories(sqlite3 PUBLIC ${PROJECT_SOURCE_DIR}/sqlite)
    list(APPEND PERSISTENCE_EXTRA_SOURCES src/persistence/SQLiteRepository.cpp)
    list(APPEND PERSISTENCE_EXTRA_LIBS sqlite3)
endif()

add_library(persistence STATIC ${PERSISTENCE_SOURCES} ${PERSISTENCE_EXTRA_SOURCES})
target_include_directories(persistence PUBLIC ${INCLUDE_DIR} ${PROJECT_SOURCE_DIR}/sqlite)
target_link_libraries(persistence PUBLIC core ${PERSISTENCE_EXTRA_LIBS})
if(WITH_SQLITE)
    target_compile_definitions(persistence PUBLIC FREEBUFF_WITH_SQLITE)
endif()

#  7.  libui.a
add_library(ui STATIC ${UI_SOURCES})
target_include_directories(ui PUBLIC ${INCLUDE_DIR})
target_link_libraries(ui PUBLIC core)

#  8.  libmysql_wrapper.a
if(WITH_MYSQL)
    if(WIN32)
        find_path(MYSQL_INCLUDE_DIR NAMES mysql.h
            PATHS "C:/xampp/mysql/include" "C:/Program Files/MySQL/MySQL Server 8.0/include"
                  "C:/Program Files/MySQL/MySQL Server 8.4/include"
            NO_DEFAULT_PATH)
        find_library(MYSQL_LIBRARY NAMES libmysql libmariadb mysqlclient
            PATHS "C:/xampp/mysql/lib" "C:/Program Files/MySQL/MySQL Server 8.0/lib"
            NO_DEFAULT_PATH)
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(MYSQL libmariadb QUIET)
            if(NOT MYSQL_FOUND) pkg_check_modules(MYSQL mysqlclient QUIET) endif()
        endif()
        if(NOT MYSQL_FOUND)
            find_path(MYSQL_INCLUDE_DIR NAMES mysql.h
                PATHS /usr/include/mariadb /usr/include/mysql)
            find_library(MYSQL_LIBRARY NAMES mariadb mysqlclient
                PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
        endif()
    endif()
    if(MYSQL_INCLUDE_DIR OR MYSQL_INCLUDE_DIRS)
        add_library(mysql_wrapper STATIC src/mysql_wrapper/MySQLWrapper.cpp)
        target_include_directories(mysql_wrapper PUBLIC ${INCLUDE_DIR} ${MYSQL_INCLUDE_DIR} ${MYSQL_INCLUDE_DIRS})
        target_link_libraries(mysql_wrapper PUBLIC core)
        if(MYSQL_LIBRARY) target_link_libraries(mysql_wrapper PUBLIC ${MYSQL_LIBRARY}) endif()
        target_compile_definitions(mysql_wrapper PUBLIC FREEBUFF_WITH_MYSQL)
    else()
        message(WARNING "MySQL not found - building stub mysql_wrapper")
        file(WRITE "${CMAKE_BINARY_DIR}/mysql_wrapper_stub.cpp"
            "#include \"mysql_wrapper/MySQLWrapper.h\"\n"
            "namespace freebuff::db {\n"
            "MySQLWrapper::MySQLWrapper(MySQLConfig) {}\n"
            "MySQLWrapper::~MySQLWrapper() {}\n"
            "void MySQLWrapper::connect() {}\n"
            "void MySQLWrapper::disconnect() {}\n"
            "bool MySQLWrapper::ping() { return false; }\n"
            "MySQLResult MySQLWrapper::query(const std::string&) { return {}; }\n"
            "uint64_t MySQLWrapper::execute(const std::string&) { return 0; }\n"
            "std::string MySQLWrapper::escape(const std::string& r) { return r; }\n"
            "void MySQLWrapper::createDatabaseIfNotExists() {}\n"
            "void MySQLWrapper::createTablesIfNotExists() {}\n"
            "void MySQLWrapper::beginTransaction() {}\n"
            "void MySQLWrapper::commit() {}\n"
            "void MySQLWrapper::rollback() {}\n"
            "const char* MySQLWrapper::error() const noexcept { return \"MySQL not available\"; }\n"
            "}\n")
        add_library(mysql_wrapper STATIC "${CMAKE_BINARY_DIR}/mysql_wrapper_stub.cpp")
        target_include_directories(mysql_wrapper PUBLIC ${INCLUDE_DIR})
        target_link_libraries(mysql_wrapper PUBLIC core)
    endif()
endif()

#  9.  libcrypto.a
if(WITH_CRYPTO)
    add_library(crypto STATIC ${CRYPTO_SOURCES})
    target_include_directories(crypto PUBLIC ${INCLUDE_DIR})
endif()

#  10. libauth.a
if(WITH_AUTH)
    add_library(auth STATIC ${AUTH_SOURCES})
    target_include_directories(auth PUBLIC ${INCLUDE_DIR})
    target_link_libraries(auth PUBLIC crypto nlohmann_json::nlohmann_json)
endif()

#  11. libbackend.a
if(WITH_BACKEND)
    add_library(backend STATIC ${BACKEND_SOURCES})
    target_include_directories(backend PUBLIC ${INCLUDE_DIR})
    target_link_libraries(backend PUBLIC
        auth payment hardware persistence
        qr_sim nfc_sim
        nlohmann_json::nlohmann_json
        Threads::Threads ${SOCKET_LIBS})
endif()

# ═══════════════════════════════════════════════════════════════════════════════
#  Executable
# ═══════════════════════════════════════════════════════════════════════════════
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ui payment hardware persistence)
if(WITH_QR_SIM)    target_link_libraries(${PROJECT_NAME} PRIVATE qr_sim) endif()
if(WITH_NFC_SIM)   target_link_libraries(${PROJECT_NAME} PRIVATE nfc_sim) endif()
if(WITH_MYSQL)     target_link_libraries(${PROJECT_NAME} PRIVATE mysql_wrapper)
                   target_compile_definitions(${PROJECT_NAME} PRIVATE FREEBUFF_WITH_MYSQL) endif()
if(WITH_CRYPTO)    target_link_libraries(${PROJECT_NAME} PRIVATE crypto) endif()
if(WITH_AUTH)      target_link_libraries(${PROJECT_NAME} PRIVATE auth nlohmann_json::nlohmann_json) endif()
if(WITH_BACKEND)   target_link_libraries(${PROJECT_NAME} PRIVATE backend Threads::${SOCKET_LIBS})
                   target_compile_definitions(${PROJECT_NAME} PRIVATE FREEBUFF_WITH_BACKEND) endif()
if(WITH_SQLITE)    target_link_libraries(${PROJECT_NAME} PRIVATE sqlite3)
                   target_compile_definitions(${PROJECT_NAME} PRIVATE FREEBUFF_WITH_SQLITE) endif()

# ── Tests ───────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Show summary ────────────────────────────────────────────────────────────
message(STATUS "═══════════════════════════════════════════")
message(STATUS " FreeBuff Payment System  v${PROJECT_VERSION}")
message(STATUS "═══════════════════════════════════════════")
message(STATUS " C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS " WITH_BACKEND: ${WITH_BACKEND}  (HTTP server)")
message(STATUS " WITH_AUTH:    ${WITH_AUTH}  (JWT auth)")
message(STATUS " WITH_CRYPTO:  ${WITH_CRYPTO}  (SHA-256/HMAC)")
message(STATUS " BUILD_TESTS:  ${BUILD_TESTS}")
message(STATUS " Libraries:    libs/")
