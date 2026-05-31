#ifndef FREEBUFF_TERMINALUI_H
#define FREEBUFF_TERMINALUI_H

#include "IUI.h"
#include <iostream>

namespace freebuff::ui {

class TerminalUI : public IUI {
public:
    explicit TerminalUI(std::ostream& output = std::cout, std::istream& input = std::cin);

    void showWelcome() const override;
    void showGoodbye() const override;
    int showMenu(const std::vector<MenuOption>& options) const override;
    std::string promptInput(const std::string& prompt) const override;
    Money promptAmount() const override;
    void showMessage(const std::string& message) const override;
    void showError(const std::string& error) const override;
    void showResult(const PaymentResult& result) const override;
    bool confirmAction(const std::string& action) const override;
    void clearScreen() const override;
    void showProgress(const std::string& task, int percent) const override;

private:
    std::ostream& output_;
    std::istream& input_;

    void printSeparator(char ch = '=') const;
    void printHeader(const std::string& title) const;
};

} // namespace freebuff::ui

#endif
