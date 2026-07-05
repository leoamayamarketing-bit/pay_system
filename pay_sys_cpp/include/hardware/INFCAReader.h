#ifndef FREEBUFF_INFCAREADER_H
#define FREEBUFF_INFCAREADER_H

#include <string>
#include <optional>
#include "payment/NFCPayment.h"

namespace freebuff::hw {

class INFCAReader {
public:
    virtual ~INFCAReader() = default;

    virtual bool detectCard() = 0;
    virtual std::optional<NFCData> readCard() = 0;
    virtual bool processPayment(const Money& amount) = 0;
    virtual std::string readerModel() const noexcept = 0;
    virtual bool isConnected() const noexcept = 0;
};

} // namespace freebuff::hw

#endif
