#ifndef FREEBUFF_PAYMENTFACTORY_H
#define FREEBUFF_PAYMENTFACTORY_H

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "core/IPaymentMethod.h"

namespace freebuff {

class PaymentFactory {
public:
    using PaymentCreator = std::function<std::unique_ptr<IPaymentMethod>()>;

    static PaymentFactory& instance();

    void registerMethod(const std::string& name, PaymentCreator creator);

    std::unique_ptr<IPaymentMethod> create(const std::string& name) const;

    [[nodiscard]] std::vector<std::string> availableMethods() const;

    bool unregisterMethod(const std::string& name);

private:
    PaymentFactory() = default;
    PaymentFactory(const PaymentFactory&) = delete;
    PaymentFactory& operator=(const PaymentFactory&) = delete;

    struct CreatorEntry {
        std::string name;
        PaymentCreator creator;
    };
    std::vector<CreatorEntry> creators_;
};

} // namespace freebuff

#endif
