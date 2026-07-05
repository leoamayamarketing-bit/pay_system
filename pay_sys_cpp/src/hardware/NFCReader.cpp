#include "hardware/NFCReader.h"
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace freebuff::hw {

NFCReader::NFCReader()
    : model_("FreeBuff NFC Reader v1.0")
    , rng_(std::random_device{}())
{}

NFCReader::NFCReader(const std::string& model)
    : model_(model)
    , rng_(std::random_device{}())
{}

bool NFCReader::detectCard() {
    if (!connected_) return false;
    
    // Simulate NFC field detection
    // 30% chance of detecting a card when not explicitly set
    if (!cardPresent_) {
        std::uniform_int_distribution<int> dist(0, 100);
        cardPresent_ = (dist(rng_) < 30);
    }
    
    return cardPresent_;
}

std::optional<NFCData> NFCReader::readCard() {
    if (!connected_ || !cardPresent_) {
        return std::nullopt;
    }
    
    NFCData data;
    data.cardUid = generateUID();
    data.cardType = randomCardType();
    
    // Generate masked PAN
    std::uniform_int_distribution<int> digitDist(0, 9);
    std::stringstream pan;
    for (int i = 0; i < 16; ++i) {
        if (i < 6 || i >= 12) {
            pan << digitDist(rng_);
        } else {
            pan << '*';
        }
        if (i % 4 == 3 && i < 15) pan << ' ';
    }
    data.maskedPan = pan.str();
    
    return data;
}

bool NFCReader::processPayment(const Money& /*amount*/) {
    if (!connected_ || !cardPresent_) return false;
    
    // Simulate payment processing delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 95% success rate simulation
    std::uniform_int_distribution<int> dist(0, 100);
    bool success = (dist(rng_) < 95);
    
    if (success) {
        cardPresent_ = false; // Card removed after processing
    }
    
    return success;
}

std::string NFCReader::generateUID() {
    std::uniform_int_distribution<int> dist(0, 255);
    std::stringstream ss;
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ss << ":";
        ss << std::hex << std::setw(2) << std::setfill('0') << dist(rng_);
    }
    return ss.str();
}

NFCCardType NFCReader::randomCardType() {
    std::uniform_int_distribution<int> dist(0, 2);
    switch (dist(rng_)) {
        case 0: return NFCCardType::Visa;
        case 1: return NFCCardType::Mastercard;
        case 2: return NFCCardType::AmericanExpress;
        default: return NFCCardType::Unknown;
    }
}

} // namespace freebuff::hw
