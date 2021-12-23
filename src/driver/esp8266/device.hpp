#include "driver/device.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "ESP8266HTTPClient.h"
#include <umm_malloc/umm_heap_select.h>

namespace driver {
HeapSelectIram::HeapSelectIram() noexcept: ptr(new (std::nothrow) ::HeapSelectIram()) {
  iop_assert(ptr, FLASH("Unable to allocate HeapSelectIram"));
}
HeapSelectIram::~HeapSelectIram() noexcept { delete this->ptr; }
HeapSelectDram::HeapSelectDram() noexcept: ptr(new (std::nothrow) ::HeapSelectDram()) {
  iop_assert(ptr, FLASH("Unable to allocate HeapSelectDram"));
}
HeapSelectDram::~HeapSelectDram() noexcept {
  delete this->ptr;
}

void Device::syncNTP() const noexcept {
  // UTC by default, should we change according to the user? We currently only use this to validate SSL cert dates
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}
auto Device::platform() const noexcept -> ::iop::StaticString {
  return FLASH("ESP8266");
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
  const auto hashed = std::string_view(hashedRaw.c_str());
  if (hashed.length() != 32) {
    iop_panic(FLASH("MD5 hex size is not 32, this is critical: ").toString() + std::string(hashed));
  }
  if (!iop::isAllPrintable(hashed)) {
    iop_panic(FLASH("Unprintable char in MD5 hex, this is critical: ").toString() + std::string(hashed));
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