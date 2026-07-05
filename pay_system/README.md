# FreeBuff Embedded Payment System

**Professional C++20 modular payment system** – Strategy & Factory design patterns for embedded payment processing.  
Fully cross-platform: **Windows** (XAMPP/MinGW/MSVC) and **Linux** (GCC/Clang + MariaDB).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        FreeBuffApp                         │
│  main.cpp  ← links all 8 static libraries                  │
├──────────┬──────────┬──────────┬──────────┬────────────────┤
│ libui.a  │ libpayment.a │ libhardware.a │ libpersistence.a │
│ Terminal │ CreditCard  │ USBManager   │ TransactionRepo │
│   UI     │ DebitCard   │ HardwareAbs  │ Logger          │
│          │ QRPayment   │              │                 │
│          │ NFCPayment  │              │                 │
│          │ PaymentFcty │              │                 │
├──────────┼──────────┬──────────┼──────────┼────────────────┤
│ libcore.a  │ libqr_sim.a │ libnfc_sim.a │ libmysql_wrapper.a │
│ Money     │ QR ASCII   │ NFC Reader  │ MySQL/MariaDB   │
│ PaymentRes│ generation │ simulation  │ RAII wrapper    │
│ Transact. │            │             │                 │
│ IPayment  │            │             │                 │
└──────────┴──────────┴──────────┴──────────┴────────────────┘
```

### Layers (fully decoupled)

| Layer | Library | Description |
|-------|---------|-------------|
| **UI** | `libui.a` | Terminal-based interface (`ITerminalUI` / `TerminalUI`) |
| **Payment** | `libpayment.a` | Strategy pattern with abstract `IPaymentMethod` |
| **Hardware** | `libhardware.a` | USB device management, hardware abstraction |
| **NFC Simulation** | `libnfc_sim.a` | Contactless card reader simulation |
| **QR Simulation** | `libqr_sim.a` | ASCII QR code generation & payload encode/decode |
| **Persistence** | `libpersistence.a` | File-based & MySQL transaction storage |
| **MySQL** | `libmysql_wrapper.a` | Cross-platform RAII MySQL/MariaDB wrapper |
| **Core** | `libcore.a` | Domain types: `Money`, `PaymentResult`, `Transaction` |
| **Crypto** | `libcrypto.a` | SHA-256 hashing, HMAC-SHA256 for JWT signing |
| **Auth** | `libauth.a` | JWT creation/verification, user authentication service |
| **Backend** | `libbackend.a` | Embedded HTTP server, REST API, Dashboard analytics, MercadoPago webhooks |

---

## REST API (Web Server Mode)

Start with `--server` or `--server <port>`:

```bash
./FreeBuffPaymentSystem --server 8080
```

Or select option **0** from the interactive menu to launch the web server on port 8080.

### API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | Server health & version |
| GET | `/api/payment/methods` | Available payment methods |
| POST | `/api/payment/create` | Process a payment |
| GET | `/api/payment/status/:id` | Check transaction status |
| POST | `/api/payment/webhook` | Payment webhook callback |
| GET | `/api/qr/generate` | Generate QR code payload |
| GET | `/api/transactions` | List recent transactions |
| GET | `/api/dashboard/stats` | Transaction statistics |
| GET | `/api/dashboard/revenue` | Revenue by day |
| POST | `/api/auth/login` | JWT user login |
| POST | `/api/auth/register` | Register new user |
| POST | `/api/mercadopago/webhook` | MercadoPago webhook handler |
| GET | `/api/devices` | List USB devices |
| GET | `/api/hardware/status` | Hardware status report |
| POST | `/api/hardware/nfc/simulate` | Simulate NFC card presence |

### Default Users (development)

| Username | Password | Role |
|----------|----------|------|
| `admin` | `admin123` | admin |
| `merchant1` | `merchant123` | merchant |
| `demo` | `demo123` | merchant |

### Example: Health Check

```bash
curl http://localhost:8080/api/health
# {"status":"ok","server":"FreeBuff Payment System","version":"2.0.0",...}
```

### Example: Create Payment

```bash
curl -X POST http://localhost:8080/api/payment/create \
  -H "Content-Type: application/json" \
  -d '{"method":"Credit Card","amount":29.99}'
```

### Example: Login (JWT)

```bash
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin123"}'
```

---

## Payment Methods

1. **Credit Card** — Luhn algorithm validation, expiration date, CVV (3/4-digit) verification
2. **Debit Card** — Balance verification and deduction
3. **QR Payment** — Payload encoding/decoding, ASCII QR code visualization
4. **NFC Payment** — Contactless card simulation with UID and card type detection

---

## Design Patterns

- **Strategy Pattern** — Each payment method implements `IPaymentMethod` interface
- **Factory Pattern** — `PaymentFactory` (Singleton) creates payment method instances
- **RAII** — Smart pointers (`std::unique_ptr`) for all resource management
- **No explicit new/delete** — Exclusive use of modern C++ RAII
- **Abstract Interfaces** — Pure virtual classes with virtual destructors

---

## Prerequisites

### Linux (Ubuntu/Debian)

```bash
# Essential build tools
sudo apt update
sudo apt install build-essential cmake g++-11 clang-14

# MySQL/MariaDB support (optional)
sudo apt install libmariadb-dev libmariadb++-dev

# QR code support (optional)
sudo apt install libqrencode-dev

# Profiling & debugging
sudo apt install valgrind gprof gdb
```

### Windows (MSYS2 / MinGW)

```bash
# MSYS2 UCRT64 terminal
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake make

# XAMPP MySQL (optional): download from https://www.apachefriends.org/
# Default path: C:/xampp/mysql/
```

### Windows (Visual Studio)

```bash
# Install "Desktop development with C++" workload via Visual Studio Installer
# MySQL Connector/C: https://dev.mysql.com/downloads/connector/c/
```

---

## Build Instructions

### Quick build (both platforms)

```bash
mkdir build && cd build
cmake .. -DWITH_MYSQL=ON
cmake --build .
```

### Build with all modules

```bash
cmake .. \
  -DWITH_MYSQL=ON \    # Enable MySQL wrapper (auto-detects XAMPP or system lib)
  -DWITH_NFC_SIM=ON \  # Enable NFC simulation library
  -DWITH_QR_SIM=ON \   # Enable QR simulation library
  -DBUILD_TESTS=ON     # Build unit tests
cmake --build .
```

### Linux-specific MySQL paths

If CMake doesn't find MySQL automatically:

```bash
cmake .. \
  -DMYSQL_INCLUDE_DIR=/usr/include/mariadb \
  -DMYSQL_LIBRARY=/usr/lib/x86_64-linux-gnu/libmariadb.a
```

### Windows-specific XAMPP paths

CMake searches `C:/xampp/mysql/` automatically. To use a custom path:

```bash
cmake .. -G "MinGW Makefiles" \
  -DMYSQL_INCLUDE_DIR="D:/xampp/mysql/include" \
  -DMYSQL_LIBRARY="D:/xampp/mysql/lib/libmysql.lib"
```

### Build without MySQL (fallback)

```bash
cmake .. -DWITH_MYSQL=OFF
# mysql_wrapper becomes a stub library (all ops no-ops)
```

### Using real QR encoding (libqrencode)

```bash
cmake .. -DUSE_LIBQRENCODE=ON
```

---

## Output Structure

After a successful build:

```
FreeBuffPaymentSystem/
├── libs/                          # Static libraries
│   ├── libcore.a                  #   (or core.lib on MSVC)
│   ├── libhardware.a
│   ├── libpayment.a
│   ├── libpersistence.a
│   ├── libmysql_wrapper.a
│   ├── libui.a
│   ├── libnfc_sim.a
│   └── libqr_sim.a
├── build/
│   └── FreeBuffPaymentSystem.exe  # Main executable
├── include/                       # Public headers
└── src/                           # Source files
```

---

## Run

```bash
# From build directory
./FreeBuffPaymentSystem

# Or from project root
./build/FreeBuffPaymentSystem
```

### Expected output

```
============================================================
  FreeBuff Embedded Payment System
============================================================

  Welcome to the FreeBuff Payment Terminal
  Secure | Reliable | Multi-Method

  Version 1.0.0 - C++20 Architecture
------------------------------------------------------------

============================================================
  Payment Methods
============================================================
  [1] Credit Card
       Credit Card [************1111] exp 12/2028
  [2] Debit Card
       Debit Card [************3456] Bank: BANK001 Balance: $1500.00
  [3] QR Payment
       QR Payment to FreeBuff Store (Ref: ORDER-2024-001)
  [4] NFC Payment
       NFC Payment [Visa] UID: A1:B2:C3:D4 PAN: 411111******1111
  [5] NFC Payment (Hardware Demo)
       Simulate NFC card reader
  [6] USB Devices
       List simulated USB devices
  [7] Hardware Report
       Show hardware status
  [8] Transaction History
       View past transactions
  [9] MySQL Database Test
       Test MySQL connection & tables

  [0] Exit

Select option:
```

---

## Run Tests

```bash
# With CTest
cd build && ctest --output-on-failure

# Or directly
./build/test_luhn
./build/test_payment
```

---

## Library Usage (from external projects)

```cmake
# In your CMakeLists.txt:
find_library(FREEBUFF_CORE    core      PATHS /path/to/libs)
find_library(FREEBUFF_PAYMENT payment   PATHS /path/to/libs)
# ... etc.

target_include_directories(my_app PRIVATE /path/to/include)
target_link_libraries(my_app PRIVATE
    ${FREEBUFF_CORE}
    ${FREEBUFF_PAYMENT}
    ${FREEBUFF_HARDWARE}
    ${FREEBUFF_PERSISTENCE}
    ${FREEBUFF_UI}
    ${FREEBUFF_NFC_SIM}
    ${FREEBUFF_QR_SIM}
    ${FREEBUFF_MYSQL_WRAPPER}
)
```

Or install system-wide:

```bash
cmake --install . --prefix /usr/local
# Headers → /usr/local/include/
# Libraries → /usr/local/lib/
```

---

## Cross-Platform Notes

### MySQL on Windows (XAMPP)
- `libmysql.dll` must be in the PATH or alongside the `.exe`
- Headers: `C:/xampp/mysql/include/mysql.h`
- Static lib: `C:/xampp/mysql/lib/libmysql.lib`
- Connection: `localhost:3306`, user `root`, no password

### MySQL on Linux (MariaDB)
- Package: `sudo apt install libmariadb-dev`
- Headers: `/usr/include/mariadb/mysql.h`
- Static lib: `/usr/lib/x86_64-linux-gnu/libmariadb.a`
- Connection: `localhost:3306`, user `root`, password configured in Linux

### Platform Detection (automatic)

```cpp
#ifdef _WIN32
    // Windows: XAMPP MySQL, Winsock, SetConsoleTextAttribute
    #include <winsock2.h>
    #include <mysql.h>
    #pragma comment(lib, "libmysql.lib")
#else
    // Linux: native MariaDB, ANSI escape codes
    #include <mariadb/mysql.h>  // or <mysql/mysql.h>
#endif
```

---

## Hardware Simulation

The hardware layer simulates:
- **NFC card reader** (`libnfc_sim.a`) — card detection, reading, payment processing
- **USB device listing** — simulated `lsusb` output with 6 default devices
- **QR code generation** (`libqr_sim.a`) — ASCII visualization

To replace with real hardware:
1. Implement `INFCAReader` interface (in `nfc_sim/`)
2. Implement `IUSBDevice` interface (in `hardware/`)
3. Link your real implementation instead of the simulation libraries

---

## Profiling & Debugging

### Valgrind (memory leaks)

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
valgrind --leak-check=full --show-leak-kinds=all ./FreeBuffPaymentSystem
```

### GProf (performance profiling)

```bash
cmake .. -DCMAKE_CXX_FLAGS="-pg -g -O2"
make
./FreeBuffPaymentSystem
gprof ./FreeBuffPaymentSystem gmon.out > analysis.txt
```

### GDB / LLDB

```bash
gdb ./FreeBuffPaymentSystem
(gdb) run
(gdb) bt  # backtrace on crash
```

---

## Project Structure

```
FreeBuffPaymentSystem/
├── CMakeLists.txt                 # Build system (8 static libs + exe)
├── README.md                      # This file
├── include/                       # Public headers (all modules)
│   ├── core/                      #   Core domain types
│   ├── payment/                   #   Payment method interfaces
│   ├── hardware/                  #   Hardware abstraction
│   ├── nfc_sim/                   #   NFC simulation
│   ├── qr_sim/                    #   QR simulation
│   ├── persistence/               #   Persistence interfaces
│   ├── ui/                        #   UI interfaces
│   └── mysql_wrapper/             #   MySQL wrapper
├── src/                           # Source files
│   ├── main.cpp                   #   Application entry point
│   ├── core/                      #   Core implementations
│   ├── payment/                   #   Payment implementations
│   ├── hardware/                  #   Hardware implementations
│   ├── nfc_sim/                   #   NFC simulation source
│   ├── qr_sim/                    #   QR simulation source
│   ├── persistence/               #   Persistence implementations
│   ├── ui/                        #   UI implementations
│   └── mysql_wrapper/             #   MySQL wrapper source
├── tests/                         # Unit tests
│   ├── CMakeLists.txt
│   ├── test_luhn.cpp
│   └── test_payment.cpp
├── libs/                          # Output: static libraries
└── build/                         # Build artifacts
```

---

## Extending

| Task | What to do |
|------|-----------|
| Add payment method | Implement `IPaymentMethod`, register in `PaymentFactory` |
| Replace NFC sim | Implement `INFCAReader`, link instead of `nfc_sim` |
| Replace QR sim | Link `libqrencode`, enable `USE_LIBQRENCODE` |
| Add real USB | Implement `IUSBDevice` |
| New persistence | Implement `ITransactionRepository` |
| Custom UI | Implement `IUI` |
| Real database | Use `MySQLWrapper` or extend with PostgreSQL wrapper |

---

## License

MIT — FreeBuff Embedded Payment System
