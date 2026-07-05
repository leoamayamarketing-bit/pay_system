#include "persistence/TransactionRepository.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <iomanip>

namespace freebuff::persist {

TransactionRepository::TransactionRepository()
    : filePath_("transactions.dat")
{
    loadFromFile();
}

TransactionRepository::TransactionRepository(const std::string& filePath)
    : filePath_(filePath)
{
    loadFromFile();
}

TransactionRepository::~TransactionRepository() {
    saveToFile();
}

bool TransactionRepository::save(const Transaction& transaction) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(transactions_.begin(), transactions_.end(),
        [&](const Transaction& t) { return t.id == transaction.id; });
    
    if (it != transactions_.end()) {
        *it = transaction;
    } else {
        transactions_.push_back(transaction);
    }
    
    return saveToFile();
}

std::optional<Transaction> TransactionRepository::findById(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(transactions_.begin(), transactions_.end(),
        [&](const Transaction& t) { return t.id == id; });
    
    if (it != transactions_.end()) {
        return *it;
    }
    return std::nullopt;
}

std::vector<Transaction> TransactionRepository::findAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return transactions_;
}

std::vector<Transaction> TransactionRepository::findByDateRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Transaction> result;
    std::copy_if(transactions_.begin(), transactions_.end(), std::back_inserter(result),
        [&](const Transaction& t) {
            return t.timestamp >= start && t.timestamp <= end;
        });
    return result;
}

bool TransactionRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find_if(transactions_.begin(), transactions_.end(),
        [&](const Transaction& t) { return t.id == id; });
    
    if (it != transactions_.end()) {
        transactions_.erase(it);
        return saveToFile();
    }
    return false;
}

bool TransactionRepository::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    transactions_.clear();
    return saveToFile();
}

bool TransactionRepository::loadFromFile() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream file(filePath_, std::ios::binary);
    if (!file.is_open()) {
        generateSampleData();
        return false;
    }
    
    transactions_.clear();
    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));
    
    for (size_t i = 0; i < count && file; ++i) {
        Transaction tx;
        size_t strLen;
        
        file.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));
        tx.id.resize(strLen);
        file.read(tx.id.data(), strLen);
        
        file.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));
        tx.paymentMethod.resize(strLen);
        file.read(tx.paymentMethod.data(), strLen);
        
        int64_t cents;
        file.read(reinterpret_cast<char*>(&cents), sizeof(cents));
        tx.amount = Money::fromCents(cents);
        
        int statusVal;
        file.read(reinterpret_cast<char*>(&statusVal), sizeof(statusVal));
        tx.status = static_cast<PaymentStatus>(statusVal);
        
        file.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));
        tx.description.resize(strLen);
        file.read(tx.description.data(), strLen);
        
        auto duration = std::chrono::system_clock::duration::zero();
        file.read(reinterpret_cast<char*>(&duration), sizeof(duration));
        tx.timestamp = std::chrono::system_clock::time_point(duration);
        
        transactions_.push_back(std::move(tx));
    }
    
    return true;
}

bool TransactionRepository::saveToFile() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream file(filePath_, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) return false;
    
    size_t count = transactions_.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));
    
    for (const auto& tx : transactions_) {
        size_t strLen = tx.id.size();
        file.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
        file.write(tx.id.data(), strLen);
        
        strLen = tx.paymentMethod.size();
        file.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
        file.write(tx.paymentMethod.data(), strLen);
        
        int64_t cents = tx.amount.cents();
        file.write(reinterpret_cast<const char*>(&cents), sizeof(cents));
        
        int statusVal = static_cast<int>(tx.status);
        file.write(reinterpret_cast<const char*>(&statusVal), sizeof(statusVal));
        
        strLen = tx.description.size();
        file.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
        file.write(tx.description.data(), strLen);
        
        auto duration = tx.timestamp.time_since_epoch();
        file.write(reinterpret_cast<const char*>(&duration), sizeof(duration));
    }
    
    return true;
}

void TransactionRepository::generateSampleData() {
    // Generate some sample transactions for demo purposes
    auto now = std::chrono::system_clock::now();
    
    transactions_.push_back({
        "CC-123456789abc", "Credit Card", Money::fromDollars(49.99),
        PaymentStatus::Success, "Purchase at TechStore",
        now - std::chrono::hours(2)
    });
    
    transactions_.push_back({
        "DB-abcdef123456", "Debit Card", Money::fromDollars(125.00),
        PaymentStatus::Success, "Payment to Utility Company",
        now - std::chrono::hours(24)
    });
    
    transactions_.push_back({
        "QR-987654321fed", "QR Payment", Money::fromDollars(15.50),
        PaymentStatus::Success, "Payment to CoffeeShop",
        now - std::chrono::hours(48)
    });
}

} // namespace freebuff::persist
