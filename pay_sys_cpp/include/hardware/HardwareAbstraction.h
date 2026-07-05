#ifndef FREEBUFF_HARDWAREABSTRACTION_H
#define FREEBUFF_HARDWAREABSTRACTION_H

#include <memory>
#include "INFCAReader.h"
#include "NFCReader.h"
#include "USBDeviceManager.h"

namespace freebuff::hw {

class HardwareAbstraction {
public:
    HardwareAbstraction();

    [[nodiscard]] INFCAReader& nfcReader() noexcept { return *nfcReader_; }
    [[nodiscard]] const INFCAReader& nfcReader() const noexcept { return *nfcReader_; }

    [[nodiscard]] USBDeviceManager& usbManager() noexcept { return usbManager_; }
    [[nodiscard]] const USBDeviceManager& usbManager() const noexcept { return usbManager_; }

    void simulateCardPresent(bool present);
    void simulateNfcConnection(bool connected);

    bool initialize();
    void shutdown();

    [[nodiscard]] std::string hardwareReport() const;
    [[nodiscard]] bool isInitialized() const noexcept { return initialized_; }

private:
    std::unique_ptr<NFCReader> nfcReader_;
    USBDeviceManager usbManager_;
    bool initialized_{false};
};

} // namespace freebuff::hw

#endif
