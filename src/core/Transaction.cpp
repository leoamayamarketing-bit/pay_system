#include "core/Transaction.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace freebuff {

std::string Transaction::toString() const {
    std::time_t t = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm;
    localtime_s(&tm, &t);
    
    std::ostringstream oss;
    oss << "[TX " << id.substr(0, 8) << "] "
        << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " | "
        << paymentMethod << " | "
        << amount << " | "
        << paymentStatusToString(status);
    
    if (!description.empty()) {
        oss << " | " << description;
    }
    
    return oss.str();
}

} // namespace freebuff
