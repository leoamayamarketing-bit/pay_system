#include "ui/TerminalUI.h"
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

namespace freebuff::ui {

TerminalUI::TerminalUI(std::ostream& output, std::istream& input)
    : output_(output), input_(input)
{}

void TerminalUI::showWelcome() const {
    clearScreen();
    printHeader("FreeBuff Embedded Payment System");
    output_ << "\n";
    output_ << "  Welcome to the FreeBuff Payment Terminal\n";
    output_ << "  Secure | Reliable | Multi-Method\n";
    output_ << "\n";
    output_ << "  Version 1.0.0 - C++20 Architecture\n";
    output_ << "\n";
    printSeparator();
    output_ << "\n";
}

void TerminalUI::showGoodbye() const {
    output_ << "\n";
    printHeader("Thank You");
    output_ << "  FreeBuff Payment System shutting down...\n";
    output_ << "  Have a great day!\n";
    output_ << "\n";
}

int TerminalUI::showMenu(const std::vector<MenuOption>& options) const {
    printHeader("Payment Methods");
    
    for (const auto& opt : options) {
        output_ << "  [" << opt.id << "] " << opt.label << "\n";
        if (!opt.description.empty()) {
            output_ << "       " << opt.description << "\n";
        }
    }
    output_ << "\n  [0] Exit\n";
    output_ << "\n";
    printSeparator('-');
    output_ << "\n";
    
    int choice = -1;
    while (choice < 0 || choice > static_cast<int>(options.size())) {
        output_ << "Select option: ";
        input_ >> choice;
        if (input_.fail()) {
            input_.clear();
            std::string trash;
            input_ >> trash;
            choice = -1;
        }
    }
    
    return choice;
}

std::string TerminalUI::promptInput(const std::string& prompt) const {
    output_ << prompt << ": ";
    std::string input;
    input_ >> input;
    return input;
}

Money TerminalUI::promptAmount() const {
    double dollars = 0.0;
    output_ << "Enter amount ($): ";
    input_ >> dollars;
    if (input_.fail() || dollars <= 0) {
        input_.clear();
        std::string trash;
        input_ >> trash;
        return Money::fromDollars(0);
    }
    return Money::fromDollars(dollars);
}

void TerminalUI::showMessage(const std::string& message) const {
    output_ << "\n  >> " << message << "\n\n";
}

void TerminalUI::showError(const std::string& error) const {
    output_ << "\n  !! ERROR: " << error << "\n\n";
}

void TerminalUI::showResult(const PaymentResult& result) const {
    printSeparator('-');
    if (result.isSuccess()) {
        output_ << "  SUCCESS!\n";
        if (result.transactionId) {
            output_ << "  Transaction ID: " << *result.transactionId << "\n";
        }
    } else {
        output_ << "  FAILED: " << paymentStatusToString(result.status) << "\n";
        output_ << "  Reason: " << result.message << "\n";
    }
    printSeparator('-');
    output_ << "\n";
}

bool TerminalUI::confirmAction(const std::string& action) const {
    output_ << action << " (y/N): ";
    std::string input;
    input_ >> input;
    return (input == "y" || input == "Y" || input == "yes" || input == "YES");
}

void TerminalUI::clearScreen() const {
#ifdef _WIN32
    std::system("cls");
#else
    std::system("clear");
#endif
}

void TerminalUI::showProgress(const std::string& task, int percent) const {
    output_ << "\r  " << task << " [";
    int barWidth = 40;
    int pos = barWidth * percent / 100;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) output_ << "=";
        else if (i == pos) output_ << ">";
        else output_ << " ";
    }
    output_ << "] " << percent << "%" << std::flush;
    if (percent == 100) output_ << "\n";
}

void TerminalUI::printSeparator(char ch) const {
    output_ << std::string(60, ch) << "\n";
}

void TerminalUI::printHeader(const std::string& title) const {
    printSeparator();
    output_ << "  " << title << "\n";
    printSeparator();
}

} // namespace freebuff::ui
