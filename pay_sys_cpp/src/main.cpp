#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <limits>

#include "core/IPaymentMethod.h"
#include "core/PaymentResult.h"
#include "core/Money.h"
#include "core/Transaction.h"

#include "payment/CreditCard.h"
#include "payment/DebitCard.h"
#include "payment/QRPayment.h"
#include "payment/NFCPayment.h"
#include "payment/PaymentFactory.h"

#include "hardware/HardwareAbstraction.h"
#include "hardware/NFCReader.h"
#include "hardware/USBDeviceManager.h"

#include "persistence/TransactionRepository.h"

#include "ui/TerminalUI.h"

using namespace freebuff;
using namespace freebuff::ui;
using namespace freebuff::hw;
using namespace freebuff::persist;

void registerPaymentMethods(PaymentFactory& factory) {
    factory.registerMethod("Credit Card", []() -> std::unique_ptr<IPaymentMethod> {
        CardInfo info{"4111111111111111", 12, 2028, "123", "John Doe"};
        return std::make_unique<CreditCard>(std::move(info));
    });

    factory.registerMethod("Debit Card", []() -> std::unique_ptr<IPaymentMethod> {
        BankAccount account{"ES1234567890", "BANK001", Money::fromDollars(1500.00)};
        return std::make_unique<DebitCard>("1234567890123456", std::move(account));
    });

    factory.registerMethod("QR Payment", []() -> std::unique_ptr<IPaymentMethod> {
        QRPayload payload{"MERCH001", "FreeBuff Store", "ORDER-2024-001", Money::fromDollars(0)};
        return std::make_unique<QRPayment>(std::move(payload));
    });

    factory.registerMethod("NFC Payment", []() -> std::unique_ptr<IPaymentMethod> {
        NFCData nfcData{"A1:B2:C3:D4", NFCCardType::Visa, "411111******1111"};
        return std::make_unique<NFCPayment>(std::move(nfcData));
    });
}

void showTransactionHistory(const TransactionRepository& repo) {
    auto transactions = repo.findAll();
    if (transactions.empty()) {
        std::cout << "  No transactions recorded yet.\n\n";
        return;
    }

    std::cout << "\n  === Recent Transactions (" << transactions.size() << ") ===\n";
    for (const auto& tx : transactions) {
        std::cout << "  " << tx.toString() << "\n";
    }
    std::cout << "\n";
}

void processQRPayment(const TerminalUI& ui, TransactionRepository& repo) {
    ui.showMessage("QR Payment Selected");

    static std::mt19937 orderRng(std::random_device{}());
    static int orderCounter = 0;
    QRPayload payload{"MERCH001", "FreeBuff Store",
        "ORDER-" + std::to_string(++orderCounter) + "-" + std::to_string(orderRng()), Money::fromDollars(0)};
    auto qrPayment = std::make_unique<QRPayment>(std::move(payload));

    auto amount = ui.promptAmount();
    if (amount.isZero() || amount.isNegative()) {
        ui.showError("Invalid amount");
        return;
    }

    ui.showProgress("Generating QR Code", 30);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto qrCode = qrPayment->generateQRCode();
    ui.showProgress("Generating QR Code", 60);

    std::cout << "\n";
    for (const auto& line : qrCode) {
        std::cout << "    " << line << "\n";
    }
    std::cout << "\n";

    ui.showProgress("Processing QR Payment", 90);
    auto result = qrPayment->process(amount);
    ui.showProgress("Processing QR Payment", 100);

    ui.showResult(result);

    if (result.isSuccess() && result.transactionId) {
        Transaction tx{*result.transactionId, "QR Payment", amount,
            PaymentStatus::Success, "QR Payment to FreeBuff Store",
            std::chrono::system_clock::now()};
        repo.save(tx);
    }
}

void processNFCPayment(const TerminalUI& ui, HardwareAbstraction& hw, TransactionRepository& repo) {
    ui.showMessage("NFC Payment Selected");

    auto& nfcReaderBase = hw.nfcReader();

    ui.showProgress("Initializing NFC Reader", 10);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    if (!nfcReaderBase.isConnected()) {
        ui.showError("NFC Reader not connected");
        return;
    }

    ui.showProgress("Waiting for NFC card...", 30);
    hw.simulateCardPresent(true);

    if (!nfcReaderBase.detectCard()) {
        ui.showError("No NFC card detected");
        hw.simulateCardPresent(false);
        return;
    }

    ui.showProgress("Reading NFC card", 50);
    auto nfcData = nfcReaderBase.readCard();
    if (!nfcData) {
        ui.showError("Failed to read NFC card");
        hw.simulateCardPresent(false);
        return;
    }

    auto nfcPayment = std::make_unique<NFCPayment>(std::move(*nfcData));

    auto amount = ui.promptAmount();
    if (amount.isZero() || amount.isNegative()) {
        ui.showError("Invalid amount");
        hw.simulateCardPresent(false);
        return;
    }

    ui.showProgress("Processing NFC Payment", 70);
    if (!nfcReaderBase.processPayment(amount)) {
        ui.showError("NFC payment processing failed");
        hw.simulateCardPresent(false);
        return;
    }

    auto result = nfcPayment->process(amount);
    ui.showProgress("Processing NFC Payment", 100);

    ui.showResult(result);

    if (result.isSuccess() && result.transactionId) {
        Transaction tx{*result.transactionId, "NFC Payment", amount,
            PaymentStatus::Success, nfcPayment->description(),
            std::chrono::system_clock::now()};
        repo.save(tx);
    }

    hw.simulateCardPresent(false);
}

void processCardPayment(const TerminalUI& ui, const std::string& methodName,
                        PaymentFactory& factory, TransactionRepository& repo) {
    ui.showMessage(methodName + " Selected");

    auto payment = factory.create(methodName);
    if (!payment) {
        ui.showError("Payment method not available");
        return;
    }

    auto validation = payment->validate();
    if (!validation.isSuccess()) {
        ui.showError("Validation failed: " + validation.message);
        return;
    }

    auto amount = ui.promptAmount();
    if (amount.isZero() || amount.isNegative()) {
        ui.showError("Invalid amount");
        return;
    }

    if (!ui.confirmAction("Process " + payment->description() + " for " + amount.toString() + "?")) {
        ui.showMessage("Payment cancelled");
        return;
    }

    ui.showProgress("Processing " + methodName, 50);
    auto result = payment->process(amount);
    ui.showProgress("Processing " + methodName, 100);

    ui.showResult(result);

    if (result.isSuccess() && result.transactionId) {
        Transaction tx{*result.transactionId, methodName, amount,
            PaymentStatus::Success, payment->description(),
            std::chrono::system_clock::now()};
        repo.save(tx);
    }
}

int main() {
    auto ui = std::make_unique<TerminalUI>();
    auto hw = std::make_unique<HardwareAbstraction>();
    auto& factory = PaymentFactory::instance();
    auto repo = std::make_unique<TransactionRepository>();

    registerPaymentMethods(factory);
    hw->initialize();

    ui->showWelcome();

    bool running = true;
    while (running) {
        auto methods = factory.availableMethods();
        std::vector<MenuOption> menuOptions;

        for (size_t i = 0; i < methods.size(); ++i) {
            auto method = factory.create(methods[i]);
            menuOptions.push_back({
                static_cast<int>(i + 1),
                methods[i],
                method ? method->description() : ""
            });
        }

        menuOptions.push_back({5, "NFC Payment (Hardware Demo)", "Simulate NFC card reader"});
        menuOptions.push_back({6, "USB Devices", "List simulated USB devices"});
        menuOptions.push_back({7, "Hardware Report", "Show hardware status"});
        menuOptions.push_back({8, "Transaction History", "View past transactions"});

        int choice = ui->showMenu(menuOptions);

        switch (choice) {
            case 0:
                running = false;
                break;
            case 1:
                processCardPayment(*ui, "Credit Card", factory, *repo);
                break;
            case 2:
                processCardPayment(*ui, "Debit Card", factory, *repo);
                break;
            case 3:
                processQRPayment(*ui, *repo);
                break;
            case 4:
                processNFCPayment(*ui, *hw, *repo);
                break;
            case 5: {
                ui->showMessage("NFC Payment (Hardware Demo)");

                auto& reader = hw->nfcReader();

                ui->showProgress("Initializing NFC Reader", 20);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));

                if (!reader.isConnected()) {
                    ui->showError("NFC Reader not connected");
                    break;
                }

                ui->showProgress("Waiting for NFC card...", 40);
                hw->simulateCardPresent(true);

                if (!reader.detectCard()) {
                    ui->showError("No NFC card detected");
                    hw->simulateCardPresent(false);
                    break;
                }

                ui->showProgress("Reading card data", 60);
                auto nfcData = reader.readCard();
                if (!nfcData) {
                    ui->showError("Failed to read NFC card");
                    hw->simulateCardPresent(false);
                    break;
                }

                auto nfcPayment = std::make_unique<NFCPayment>(std::move(*nfcData));

                auto amount = ui->promptAmount();
                if (amount.isZero() || amount.isNegative()) {
                    ui->showError("Invalid amount");
                    hw->simulateCardPresent(false);
                    break;
                }

                ui->showProgress("Processing NFC Payment", 80);
                if (!reader.processPayment(amount)) {
                    ui->showError("NFC payment processing failed");
                    hw->simulateCardPresent(false);
                    break;
                }

                auto result = nfcPayment->process(amount);
                ui->showProgress("Processing NFC Payment", 100);

                ui->showResult(result);

                if (result.isSuccess() && result.transactionId) {
                    Transaction tx{*result.transactionId, "NFC Payment", amount,
                        PaymentStatus::Success, nfcPayment->description(),
                        std::chrono::system_clock::now()};
                    repo->save(tx);
                }

                hw->simulateCardPresent(false);
                break;
            }
            case 6: {
                ui->showMessage("USB Devices");
                auto devices = hw->usbManager().listDevices();
                if (devices.empty()) {
                    ui->showMessage("No USB devices connected");
                } else {
                    std::cout << "\n";
                    for (const auto& dev : devices) {
                        std::cout << "  " << dev.toString() << "\n";
                    }
                    std::cout << "\n";
                }
                break;
            }
            case 7: {
                auto report = hw->hardwareReport();
                std::cout << "\n" << report << "\n";
                break;
            }
            case 8: {
                showTransactionHistory(*repo);
                break;
            }
            default: {
                ui->showError("Invalid option");
                break;
            }
        }

        if (running) {
            ui->showMessage("Press Enter to continue...");
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
        }
    }

    ui->showGoodbye();
    hw->shutdown();

    return 0;
}
