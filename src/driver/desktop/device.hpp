#include "driver/device.hpp"
#include <stdint.h>
#include <thread>

namespace driver {
HeapSelectIram::HeapSelectIram() noexcept {}
HeapSelectIram::~HeapSelectIram() noexcept {}
HeapSelectDram::HeapSelectDram() noexcept {}
HeapSelectDram::~HeapSelectDram() noexcept {}
void Device::syncNTP() const noexcept {}
auto Device::platform() const noexcept -> ::iop::StaticString { return FLASH("DESKTOP"); }
auto Device::vcc() const noexcept -> uint16_t { return 1; }
auto Device::availableFlash() const noexcept -> size_t { return SIZE_MAX; }
auto Device::availableStack() const noexcept -> size_t { return 1; }
auto Device::availableHeap() const noexcept -> size_t { return 1; }
auto Device::biggestHeapBlock() const noexcept -> size_t { return 1; }
void Device::deepSleep(size_t seconds) const noexcept {
  if (seconds == 0) seconds = INT32_MAX;
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}
iop::MD5Hash & Device::binaryMD5() const noexcept {
  static iop::MD5Hash hash;
  hash.fill('A');
  return hash;
}
iop::MacAddress & Device::macAddress() const noexcept {
  static iop::MacAddress mac;
  mac.fill('A');
  return mac;
}
}