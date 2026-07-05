#include "core/Money.h"
#include <iomanip>
#include <sstream>

namespace freebuff {

std::string Money::toString() const {
    std::ostringstream oss;
    oss << "$" << std::fixed << std::setprecision(2) << toDollars();
    return oss.str();
}

} // namespace freebuff
