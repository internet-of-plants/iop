#include "driver/device.hpp"

namespace driver {
HeapSelectIram::HeapSelectIram() noexcept {}
HeapSelectIram::~HeapSelectIram() noexcept {}
HeapSelectDram::HeapSelectDram() noexcept {}
HeapSelectDram::~HeapSelectDram() noexcept {}

void Device::syncNTP() const noexcept {}
auto Device::platform() const noexcept -> ::iop::StaticString { return IOP_STATIC_STRING("NOOP"); }
auto Device::vcc() const noexcept -> uint16_t { return UINT16_MAX; }
auto Device::availableFlash() const noexcept -> size_t { return 1000000; }
auto Device::availableStack() const noexcept -> size_t { return 2000; }
auto Device::availableHeap() const noexcept -> size_t { return 20000; }
auto Device::biggestHeapBlock() const noexcept -> size_t { return 2000; }
void Device::deepSleep(const size_t seconds) const noexcept { (void) seconds; }
iop::MD5Hash & Device::binaryMD5() const noexcept { static iop::MD5Hash hash; hash.fill('\0'); hash = { 'M', 'D', '5' }; return hash; }
iop::MacAddress & Device::macAddress() const noexcept { static iop::MacAddress mac; mac.fill('\0'); mac = { 'M', 'A', 'C' }; return mac; }
}