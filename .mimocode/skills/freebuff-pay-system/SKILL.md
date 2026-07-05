---
name: freebuff-pay-system
description: Use when working with the FreeBuff Embedded Payment System — C++20 project with REST API, JWT auth, SQLite/MySQL, payment processing, NFC/QR simulation. Use for building, testing, versioning, deploying, or modifying this project.
---

# FreeBuff Payment System — Development Skill

## Project Overview

C++20 modular payment system with Strategy & Factory design patterns. Two branches:

- **`pay_sys_cpp`** — v1.0 terminal-only (5 modules: core, payment, hardware, persistence, ui)
- **`pay_system`** — v2.0 with REST API, JWT, SQLite/MySQL, MercadoPago (11 modules)

## Branch Structure

```
pay_system/
├── .gitignore              # Global gitignore
├── VERSION                 # Current version (e.g., 1.0.0)
├── docs/RULES.md           # IMMUTABLE project rules
├── pay_sys_cpp/            # Branch: pay_sys_cpp (v1.0)
│   ├── .gitignore
│   ├── CMakeLists.txt
│   ├── README.md
│   ├── VERSION
│   ├── include/
│   ├── src/
│   └── tests/
└── pay_system/             # Branch: pay_system (v2.0)
    ├── .gitignore
    ├── CMakeLists.txt
    ├── README.md
    ├── VERSION
    ├── include/
    ├── src/
    ├── tests/
    └── sqlite/
```

## Build Commands

### Quick build (any branch)

```bash
cd pay_system   # or pay_sys_cpp
mkdir -p build && cd build
cmake ..
cmake --build .
```

### Build with all modules (pay_system only)

```bash
cmake .. \
  -DWITH_MYSQL=ON \
  -DWITH_NFC_SIM=ON \
  -DWITH_QR_SIM=ON \
  -DWITH_BACKEND=ON \
  -DWITH_AUTH=ON \
  -DWITH_CRYPTO=ON \
  -DWITH_SQLITE=ON \
  -DBUILD_TESTS=ON
cmake --build .
```

### Clean build

```bash
rm -rf build && mkdir build && cd build
cmake .. && cmake --build .
```

## Test Commands

```bash
cd build
ctest --output-on-failure
# Or directly:
./test_luhn
./test_payment
```

## Run

```bash
# Terminal mode
./build/FreeBuffPaymentSystem

# Server mode (pay_system only)
./build/FreeBuffPaymentSystem --server 8080
```

## Versioning Rules

**IMMUTABLE — See docs/RULES.md for full rules.**

1. Format: `v1.X.Y` — minor increments every 10 patches
2. `VERSION` file must match the last tag (without `v` prefix)
3. Each branch versions independently: `pay_sys_cpp/v1.0.0`, `pay_system/v1.0.0`
4. Tag naming: `<branch-name>/v<version>` for independent versioning
5. Conventional commits: `feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`, `build:`

### Version workflow

```bash
# 1. Read current version
cat VERSION

# 2. Determine next version (patch +1, or minor +1 after patch 9)

# 3. Update VERSION file
echo "1.0.1" > VERSION

# 4. Commit
git add VERSION
git commit -m "chore: bump version to 1.0.1"

# 5. Tag
git tag -a "pay_system/v1.0.1" -m "Version 1.0.1"

# 6. Push
git push origin HEAD:pay_system --tags
```

## Git Workflow

### Switch to project branch

```bash
git checkout pay_system    # v2.0 with REST API
git checkout pay_sys_cpp   # v1.0 terminal-only
```

### Push changes

```bash
git push origin HEAD:<branch-name> --tags
```

### Create new version

```bash
# Update VERSION, commit, tag, push
echo "1.0.1" > VERSION
git add VERSION
git commit -m "chore: bump version to 1.0.1"
git tag -a "pay_system/v1.0.1" -m "Version 1.0.1"
git push origin HEAD:pay_system --tags
```

## C++ Code Rules

1. **C++20 standard** — no C++23 features
2. **RAII always** — `std::unique_ptr`/`std::shared_ptr`, no `new`/`delete`
3. **No `using namespace std`** in headers
4. **`#pragma once`** in all headers
5. **Const correctness** throughout
6. **Werror** — warnings are errors
7. **Try-catch** for file ops, DB queries, network calls
8. **`std::lock_guard`** for all mutex usage
9. **Conventional commits** for all changes

## Module Reference (pay_system)

| Module | Library | Purpose |
|--------|---------|---------|
| core | libcore.a | Money, PaymentResult, Transaction |
| payment | libpayment.a | CreditCard, DebitCard, QRPayment, NFCPayment, PaymentFactory |
| hardware | libhardware.a | USBDeviceManager, HardwareAbstraction |
| nfc_sim | libnfc_sim.a | NFCReader simulation |
| qr_sim | libqr_sim.a | QRSimulator ASCII QR |
| persistence | libpersistence.a | TransactionRepository, Logger, SQLiteRepository |
| ui | libui.a | TerminalUI |
| mysql_wrapper | libmysql_wrapper.a | MySQL/MariaDB RAII wrapper |
| crypto | libcrypto.a | SHA256, HMAC |
| auth | libauth.a | JWT, AuthService |
| backend | libbackend.a | WebServer, HttpServer, DashboardStats, MercadoPago |

## API Endpoints (pay_system server mode)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | Health check |
| POST | `/api/payment/create` | Process payment |
| GET | `/api/payment/status/:id` | Transaction status |
| POST | `/api/auth/login` | JWT login |
| GET | `/api/dashboard/stats` | Statistics |
| GET | `/api/transactions` | List transactions |

## Default Credentials (development)

| User | Password | Role |
|------|----------|------|
| admin | admin123 | admin |
| merchant1 | merchant123 | merchant |
| demo | demo123 | merchant |

## Troubleshooting

### MySQL not found

```bash
cmake .. -DWITH_MYSQL=OFF
```

### Build fails on Windows

Use MSYS2 UCRT64 terminal with mingw-w64-ucrt-x86_64-gcc.

### Tag already exists

Tags are per-branch using prefix: `pay_sys_cpp/v1.0.0` and `pay_system/v1.0.0`.

### Push timeout

Use `git push origin HEAD:<branch> --tags` with explicit branch name.
