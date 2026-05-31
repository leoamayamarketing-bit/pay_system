#ifndef FREEBUFF_MONEY_H
#define FREEBUFF_MONEY_H

#include <cstdint>
#include <string>
#include <ostream>

namespace freebuff {

class Money {
public:
    Money() noexcept = default;

    explicit Money(int64_t cents) noexcept : cents_(cents) {}

    static Money fromDollars(double dollars) noexcept {
        return Money(static_cast<int64_t>(dollars * 100.0 + 0.5));
    }

    static Money fromCents(int64_t cents) noexcept {
        return Money(cents);
    }

    [[nodiscard]] int64_t cents() const noexcept { return cents_; }

    [[nodiscard]] double toDollars() const noexcept {
        return static_cast<double>(cents_) / 100.0;
    }

    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool isZero() const noexcept { return cents_ == 0; }
    [[nodiscard]] bool isNegative() const noexcept { return cents_ < 0; }
    [[nodiscard]] bool isPositive() const noexcept { return cents_ > 0; }

    Money operator+(const Money& other) const noexcept {
        return Money(cents_ + other.cents_);
    }

    Money operator-(const Money& other) const noexcept {
        return Money(cents_ - other.cents_);
    }

    bool operator==(const Money& other) const noexcept { return cents_ == other.cents_; }
    bool operator!=(const Money& other) const noexcept { return cents_ != other.cents_; }
    bool operator<(const Money& other) const noexcept { return cents_ < other.cents_; }
    bool operator<=(const Money& other) const noexcept { return cents_ <= other.cents_; }
    bool operator>(const Money& other) const noexcept { return cents_ > other.cents_; }
    bool operator>=(const Money& other) const noexcept { return cents_ >= other.cents_; }

    friend std::ostream& operator<<(std::ostream& os, const Money& m) {
        os << m.toString();
        return os;
    }

private:
    int64_t cents_{0};
};

} // namespace freebuff

#endif // FREEBUFF_MONEY_H
