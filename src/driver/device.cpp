#include "driver/device.hpp"
#include "core/utils.hpp"
#include "loop.hpp"

namespace driver {
    Device device;
}

#ifdef IOP_DESKTOP
#include <stdint.h>
#include <thread>

namespace driver {
auto Device::platform() const noexcept -> ::iop::StaticString {
  return F("DESKTOP");
}
auto Device::vcc() const noexcept -> uint16_t {
  return 1;
}
auto Device::availableFlash() const noexcept -> size_t {
  return SIZE_MAX;
}
auto Device::availableStack() const noexcept -> size_t {
  // TODO: calculate this
  return 1;
}
auto Device::availableHeap() const noexcept -> size_t {
  return 1;
}
auto Device::biggestHeapBlock() const noexcept -> size_t {
  return 1;
}
void Device::deepSleep(uint32_t seconds) const noexcept {
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
#define sprintf_P sprintf
#else
#include "Esp.h"
#include "user_interface.h"
#include <optional>
#include "core/panic.hpp"
#include "utils.hpp"
#include <coredecls.h>
#include <umm_malloc/umm_heap_select.h>

// This breaks expectation of the "modules"
#include "utils.hpp"

namespace driver {
auto Device::platform() const noexcept -> ::iop::StaticString {
  return F("ESP8266");
}
auto Device::vcc() const noexcept -> uint16_t {
    return ESP.getVcc();
}
auto Device::availableFlash() const noexcept -> size_t {
    return ESP.getFreeSketchSpace();
}
auto Device::availableStack() const noexcept -> size_t {
    //disable_extra4k_at_link_time();
    ESP.resetFreeContStack();
    return ESP.getFreeContStack();
}
auto Device::availableHeap() const noexcept -> size_t {
    return ESP.getFreeHeap();
}
auto Device::biggestHeapBlock() const noexcept -> size_t {
    return ESP.getMaxFreeBlockSize();
}
void Device::deepSleep(const size_t seconds) const noexcept {
    ESP.deepSleep(seconds * 1000000);
}
iop::MD5Hash & Device::binaryMD5() const noexcept {
  static bool cached = false;
  if (cached)
    return iop::data.md5;

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
  const auto hashedRaw = ESP.getSketchMD5();
  const auto hashed = iop::to_view(hashedRaw);
  if (hashed.length() != 32) {
    iop_panic(iop::StaticString(F("MD5 hex size is not 32, this is critical: ")).toString() + std::string(hashed));
  }
  if (!iop::isAllPrintable(hashed)) {
    iop_panic(iop::StaticString(F("Unprintable char in MD5 hex, this is critical: ")).toString() + std::string(hashed));
  }

  memcpy(iop::data.md5.data(), hashed.begin(), 32);
  cached = true;
  
  return iop::data.md5;
}
iop::MacAddress & Device::macAddress() const noexcept {
  auto &mac = iop::data.mac;

  static bool cached = false;
  if (cached)
    return mac;
  cached = true;
  IOP_TRACE();

  std::array<uint8_t, 6> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  const auto *fmt = PSTR("%02X:%02X:%02X:%02X:%02X:%02X");
  sprintf_P(mac.data(), fmt, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  return mac;
}
}

#endif