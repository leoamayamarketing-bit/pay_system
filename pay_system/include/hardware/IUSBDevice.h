#ifndef FREEBUFF_IUSBDEVICE_H
#define FREEBUFF_IUSBDEVICE_H

#include <string>
#include <vector>
#include <ostream>

namespace freebuff::hw {

struct USBDeviceInfo {
    int busNumber{0};
    int deviceNumber{0};
    std::string vendorId;
    std::string productId;
    std::string vendorName;
    std::string productName;
    std::string driver;

    [[nodiscard]] std::string toString() const;
};

class IUSBDevice {
public:
    virtual ~IUSBDevice() = default;

    virtual USBDeviceInfo deviceInfo() const = 0;
    virtual bool connect() = 0;
    virtual bool disconnect() = 0;
    virtual bool isConnected() const noexcept = 0;
    virtual std::string devicePath() const = 0;
};

} // namespace freebuff::hw

#endif
