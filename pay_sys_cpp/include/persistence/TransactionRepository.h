#ifndef FREEBUFF_TRANSACTIONREPOSITORY_H
#define FREEBUFF_TRANSACTIONREPOSITORY_H

#include "ITransactionRepository.h"
#include <vector>
#include <mutex>

namespace freebuff::persist {

class TransactionRepository : public ITransactionRepository {
public:
    TransactionRepository();
    explicit TransactionRepository(const std::string& filePath);
    ~TransactionRepository() override;

    bool save(const Transaction& transaction) override;
    std::optional<Transaction> findById(const std::string& id) const override;
    std::vector<Transaction> findAll() const override;
    std::vector<Transaction> findByDateRange(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end) const override;
    bool remove(const std::string& id) override;
    size_t count() const noexcept override { return transactions_.size(); }
    bool clear() override;

    bool loadFromFile();
    bool saveToFile() const;

private:
    std::string filePath_;
    std::vector<Transaction> transactions_;
    mutable std::mutex mutex_;

    void generateSampleData();
};

} // namespace freebuff::persist

#endif
