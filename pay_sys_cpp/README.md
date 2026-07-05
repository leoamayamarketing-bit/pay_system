# FreeBuff Embedded Payment System

A professional C++20 modular payment system implementing **Strategy** and **Factory** design patterns for embedded payment processing.

## Architecture

```
UI (TerminalUI) → Payment Logic (Strategy Pattern) → Hardware Abstraction → Persistence
```

### Layers (fully decoupled)

| Layer | Description |
|-------|-------------|
| **UI** | Terminal-based interface (`ITerminalUI`) |
| **Payment** | Strategy pattern with abstract `IPaymentMethod` |
| **Hardware** | NFC reader, USB device simulation (`INFCAReader`, `USBDeviceManager`) |
| **Persistence** | Transaction repository with file storage (`ITransactionRepository`) |

## Payment Methods

1. **Credit Card** - Luhn algorithm validation, expiration date, CVV verification
2. **Debit Card** - Balance verification and deduction
3. **QR Payment** - Payload encoding/decoding, ASCII QR code visualization
4. **NFC Payment** - Contactless card simulation with UID and card type detection

## Design Patterns

- **Strategy Pattern** - Each payment method implements `IPaymentMethod` interface
- **Factory Pattern** - `PaymentFactory` (Singleton) creates payment method instances
- **RAII** - Smart pointers (`std::unique_ptr`) for all resource management
- **No explicit new/delete** - Exclusive use of modern C++ RAII
- **Abstract Interfaces** - Pure virtual classes with virtual destructors

## Build Instructions

### Prerequisites
- CMake 3.20+
- C++20 compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Run

```bash
./FreeBuffPaymentSystem
```

### Run Tests

```bash
ctest --output-on-failure
```

Or directly:

```bash
./build/test_luhn
./build/test_payment
```

## Hardware Simulation

The hardware layer simulates:
- NFC card reader with card detection, reading, and payment processing
- USB device listing (simulated `lsusb` output)
- Default USB devices typical for payment terminals

To replace with real hardware, implement the `INFCAReader` and `IUSBDevice` interfaces.

## Project Structure

```
include/
  core/          # Core abstractions (IPaymentMethod, Money, Transaction)
  payment/       # Payment method implementations + Factory
  hardware/      # Hardware interfaces and simulations
  persistence/   # Data persistence interfaces
  ui/            # User interface
src/
  main.cpp
  core/          # Core implementations
  payment/       # Payment implementations
  hardware/      # Hardware implementations
  persistence/   # Persistence implementations
  ui/            # UI implementations
tests/           # Unit tests
```

## QR Code Note

QR generation uses a built-in ASCII representation. For production use, install `libqrencode` and enable the `USE_LIBQRENCODE` CMake option:

```bash
cmake -DUSE_LIBQRENCODE=ON ..
```
