#ifndef FREEBUFF_NFCREADER_H
#define FREEBUFF_NFCREADER_H

#include "INFCAReader.h"
#include <random>
#include <string>

namespace freebuff::hw {

class NFCReader : public INFCAReader {
public:
    NFCReader();
    explicit NFCReader(const std::string& model);

    bool detectCard() override;
    std::optional<NFCData> readCard() override;
    bool processPayment(const Money& amount) override;
    std::string readerModel() const noexcept override { return model_; }
    bool isConnected() const noexcept override { return connected_; }

    void simulateCardPresent(bool present) { cardPresent_ = present; }
    void simulateConnection(bool connected) { connected_ = connected; }

private:
    std::string  model_;
    bool         connected_{true};
    bool         cardPresent_{false};
    std::mt19937 rng_;

    std::string   generateUID();
    NFCCardType   randomCardType();
};

} // namespace freebuff::hw
#endif
