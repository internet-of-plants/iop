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
auto Device::vcc() const noexcept -> uint16_t {
    return SIZE_MAX;
}
auto Device::availableFlash() const noexcept -> size_t {
  return SIZE_MAX;
}
auto Device::availableStack() const noexcept -> size_t {
  // TODO: calculate this
  return 1;
}
auto Device::availableHeap() const noexcept -> size_t {
  return SIZE_MAX;
}
auto Device::biggestHeapBlock() const noexcept -> size_t {
  return SIZE_MAX;
}
void Device::deepSleep(uint32_t seconds) const noexcept {
  if (seconds == 0) seconds = INT32_MAX;
  std::this_thread::sleep_for(std::chrono::seconds(seconds));
}
iop::MD5Hash & Device::binaryMD5() const noexcept {
  static std::optional<iop::MD5Hash> hash;
  if (hash.has_value())
    return iop::unwrap_mut(hash, IOP_CTX());
  // TODO: actually hash desktop binary that is being run
  hash = iop::ok(iop::MD5Hash::fromString("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  return iop::unwrap_mut(hash, IOP_CTX());
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
// This breaks expectation of the "modules"
#include "utils.hpp"

namespace driver {
auto Device::vcc() const noexcept -> uint16_t {
    return ESP.getVcc();
}
auto Device::availableFlash() const noexcept -> size_t {
    return ESP.getFreeSketchSpace();
}
auto Device::availableStack() const noexcept -> size_t {
    disable_extra4k_at_link_time();
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
    return unused4KbSysStack.md5();

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
  const auto hashedRaw = ESP.getSketchMD5();
  const auto hashed = iop::to_view(hashedRaw);
  if (hashed.length() != 32) {
    iop_panic(iop::StaticString(F("MD5 hex size is not 32, this is critical: ")).toStdString() + std::string(hashed));
  }
  if (!iop::isAllPrintable(hashed)) {
    iop_panic(iop::StaticString(F("Unprintable char in MD5 hex, this is critical: ")).toStdString() + std::string(hashed));
  }

  memcpy(unused4KbSysStack.md5().data(), hashed.begin(), 32);
  cached = true;
  
  return unused4KbSysStack.md5();
}
iop::MacAddress & Device::macAddress() const noexcept {
  IOP_TRACE();
  auto &mac = unused4KbSysStack.mac();

  static bool cached = false;
  if (cached)
    return mac;
  cached = true;
  
  std::array<uint8_t, 6> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  const auto *fmt = PSTR("%02X:%02X:%02X:%02X:%02X:%02X");
  sprintf_P(mac.data(), fmt, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  return mac;
}
}

#endif