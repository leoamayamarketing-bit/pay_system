#ifndef FREEBUFF_USBDEVICEMANAGER_H
#define FREEBUFF_USBDEVICEMANAGER_H

#include <memory>
#include <string>
#include <vector>
#include "IUSBDevice.h"

namespace freebuff::hw {

class USBDeviceManager {
public:
    USBDeviceManager();

    std::vector<USBDeviceInfo> listDevices() const;
    std::vector<USBDeviceInfo> simulateLSUSB() const;
    bool addDevice(USBDeviceInfo device);
    bool removeDevice(int busNumber, int deviceNumber);

    std::vector<USBDeviceInfo> findDevicesByVendor(const std::string& vendorId) const;
    std::vector<USBDeviceInfo> findDevicesByProduct(const std::string& productId) const;

    void clear();
    [[nodiscard]] size_t deviceCount() const noexcept { return devices_.size(); }

private:
    std::vector<USBDeviceInfo> devices_;
    void populateDefaultDevices();
};

} // namespace freebuff::hw

#endif
