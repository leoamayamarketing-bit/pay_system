#include "payment/PaymentFactory.h"
#include <algorithm>

namespace freebuff {

PaymentFactory& PaymentFactory::instance() {
    static PaymentFactory factory;
    return factory;
}

void PaymentFactory::registerMethod(const std::string& name, PaymentCreator creator) {
    auto it = std::find_if(creators_.begin(), creators_.end(),
        [&](const CreatorEntry& entry) { return entry.name == name; });
    
    if (it != creators_.end()) {
        it->creator = std::move(creator);
    } else {
        creators_.push_back({name, std::move(creator)});
    }
}

std::unique_ptr<IPaymentMethod> PaymentFactory::create(const std::string& name) const {
    auto it = std::find_if(creators_.begin(), creators_.end(),
        [&](const CreatorEntry& entry) { return entry.name == name; });
    
    if (it != creators_.end() && it->creator) {
        return it->creator();
    }
    
    return nullptr;
}

std::vector<std::string> PaymentFactory::availableMethods() const {
    std::vector<std::string> methods;
    methods.reserve(creators_.size());
    for (const auto& entry : creators_) {
        methods.push_back(entry.name);
    }
    return methods;
}

bool PaymentFactory::unregisterMethod(const std::string& name) {
    auto it = std::find_if(creators_.begin(), creators_.end(),
        [&](const CreatorEntry& entry) { return entry.name == name; });
    
    if (it != creators_.end()) {
        creators_.erase(it);
        return true;
    }
    return false;
}

} // namespace freebuff
