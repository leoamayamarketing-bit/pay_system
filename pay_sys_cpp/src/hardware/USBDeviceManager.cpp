#include "hardware/USBDeviceManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace freebuff::hw {

std::string USBDeviceInfo::toString() const {
    std::ostringstream oss;
    oss << "Bus " << std::setw(3) << busNumber
        << " Device " << std::setw(3) << deviceNumber
        << ": ID " << vendorId << ":" << productId
        << " " << vendorName << " " << productName;
    if (!driver.empty()) {
        oss << " [" << driver << "]";
    }
    return oss.str();
}

USBDeviceManager::USBDeviceManager() {
    populateDefaultDevices();
}

std::vector<USBDeviceInfo> USBDeviceManager::listDevices() const {
    return devices_;
}

std::vector<USBDeviceInfo> USBDeviceManager::simulateLSUSB() const {
    // Simulates the output format of lsusb command
    return devices_;
}

bool USBDeviceManager::addDevice(USBDeviceInfo device) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&](const USBDeviceInfo& d) {
            return d.busNumber == device.busNumber &&
                   d.deviceNumber == device.deviceNumber;
        });
    
    if (it != devices_.end()) return false;
    devices_.push_back(std::move(device));
    return true;
}

bool USBDeviceManager::removeDevice(int busNumber, int deviceNumber) {
    auto it = std::find_if(devices_.begin(), devices_.end(),
        [&](const USBDeviceInfo& d) {
            return d.busNumber == busNumber &&
                   d.deviceNumber == deviceNumber;
        });
    
    if (it == devices_.end()) return false;
    devices_.erase(it);
    return true;
}

std::vector<USBDeviceInfo> USBDeviceManager::findDevicesByVendor(
    const std::string& vendorId) const {
    std::vector<USBDeviceInfo> result;
    std::copy_if(devices_.begin(), devices_.end(), std::back_inserter(result),
        [&](const USBDeviceInfo& d) { return d.vendorId == vendorId; });
    return result;
}

std::vector<USBDeviceInfo> USBDeviceManager::findDevicesByProduct(
    const std::string& productId) const {
    std::vector<USBDeviceInfo> result;
    std::copy_if(devices_.begin(), devices_.end(), std::back_inserter(result),
        [&](const USBDeviceInfo& d) { return d.productId == productId; });
    return result;
}

void USBDeviceManager::clear() {
    devices_.clear();
}

void USBDeviceManager::populateDefaultDevices() {
    // Simulated USB devices typical for a payment terminal
    devices_.push_back({1, 1, "1a86", "7523", "QinHeng Electronics",
        "NFC Reader HC-RF01", "pn547"});
    
    devices_.push_back({1, 2, "ffff", "0035", "Generic",
        "USB Keyboard", "hid"});
    
    devices_.push_back({2, 1, "0bda", "0129", "Realtek Semiconductor",
        "USB 2.0 Card Reader", "usb-storage"});
    
    devices_.push_back({3, 1, "04e6", "e001", "SCM Microsystems",
        "Contactless Smart Card Reader", "ifd-ccid"});
    
    devices_.push_back({3, 2, "08e6", "3437", "Giesecke & Devrient",
        "PIN Pad Reader", "ifd-ccid"});
    
    devices_.push_back({4, 1, "067b", "2303", "Prolific Technology",
        "PL2303 Serial Adapter", "pl2303"});
}

} // namespace freebuff::hw
