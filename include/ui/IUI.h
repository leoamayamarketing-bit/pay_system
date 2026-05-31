#ifndef FREEBUFF_IUI_H
#define FREEBUFF_IUI_H

#include <string>
#include <vector>
#include "core/Money.h"
#include "core/PaymentResult.h"

namespace freebuff::ui {

struct MenuOption {
    int id;
    std::string label;
    std::string description;
};

class IUI {
public:
    virtual ~IUI() = default;

    virtual void showWelcome() const = 0;
    virtual void showGoodbye() const = 0;
    virtual int showMenu(const std::vector<MenuOption>& options) const = 0;
    virtual std::string promptInput(const std::string& prompt) const = 0;
    virtual Money promptAmount() const = 0;
    virtual void showMessage(const std::string& message) const = 0;
    virtual void showError(const std::string& error) const = 0;
    virtual void showResult(const PaymentResult& result) const = 0;
    virtual bool confirmAction(const std::string& action) const = 0;
    virtual void clearScreen() const = 0;
    virtual void showProgress(const std::string& task, int percent) const = 0;
};

} // namespace freebuff::ui

#endif
