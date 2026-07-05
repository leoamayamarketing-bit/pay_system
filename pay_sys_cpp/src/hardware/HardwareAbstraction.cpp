#include "hardware/HardwareAbstraction.h"
#include <sstream>

namespace freebuff::hw {

HardwareAbstraction::HardwareAbstraction()
    : nfcReader_(std::make_unique<NFCReader>())
{}

bool HardwareAbstraction::initialize() {
    if (initialized_) return true;
    
    // Simulate hardware initialization
    if (nfcReader_ && nfcReader_->isConnected()) {
        initialized_ = true;
        return true;
    }
    
    return false;
}

void HardwareAbstraction::simulateCardPresent(bool present) {
    if (nfcReader_) {
        nfcReader_->simulateCardPresent(present);
    }
}

void HardwareAbstraction::simulateNfcConnection(bool connected) {
    if (nfcReader_) {
        nfcReader_->simulateConnection(connected);
    }
}

void HardwareAbstraction::shutdown() {
    if (!initialized_) return;
    
    // Simulate hardware shutdown
    nfcReader_->simulateConnection(false);
    initialized_ = false;
}

std::string HardwareAbstraction::hardwareReport() const {
    std::ostringstream oss;
    oss << "=== Hardware Status Report ===\n";
    oss << "Status: " << (initialized_ ? "Initialized" : "Not Initialized") << "\n";
    oss << "NFC Reader: " << (nfcReader_ ? nfcReader_->readerModel() : "None") << "\n";
    oss << "NFC Connected: " << (nfcReader_ && nfcReader_->isConnected() ? "Yes" : "No") << "\n";
    
    auto devices = usbManager_.listDevices();
    oss << "USB Devices: " << devices.size() << " connected\n";
    for (const auto& dev : devices) {
        oss << "  " << dev.toString() << "\n";
    }
    
    return oss.str();
}

} // namespace freebuff::hw
