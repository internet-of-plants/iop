#include "driver/device.hpp"
#include "core/utils.hpp"

namespace driver {
    Device device;
}

#ifdef IOP_DESKTOP
#include <stdint.h>
#include <thread>

namespace driver {
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
    if (seconds == 0) seconds = SIZE_MAX;
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
  static std::optional<iop::MacAddress> mac;
  if (mac.has_value())
    return iop::unwrap_mut(mac, IOP_CTX());
  // TODO: actually get network chip address
  mac = iop::ok(iop::MacAddress::fromString("AAAAAAAAAAAAAAAAA"));
  return iop::unwrap_mut(mac, IOP_CTX());
}
}
#define sprintf_P sprintf
#else
#include "Esp.h"
#include "user_interface.h"
#include <optional>
#include "core/panic.hpp"
#include "core/utils.hpp"

namespace driver {
auto Device::availableFlash() const noexcept -> size_t {
    return ESP.getFreeSketchSpace();
}
auto Device::availableStack() const noexcept -> size_t {
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
  static std::optional<iop::MD5Hash> hash;
  if (hash.has_value())
    return iop::unwrap_mut(hash, IOP_CTX());

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
  const auto hashed = ESP.getSketchMD5();
  auto res = iop::MD5Hash::fromString(iop::to_view(hashed));
  if (iop::is_err(res)) {
    const auto &ref = iop::unwrap_err_ref(res, IOP_CTX());
    const iop::StaticString sizeErr(F("MD5 hex is too big, this is critical: "));
    const iop::StaticString printErr(F("Unprintable char in MD5 hex, this is critical: "));
    const auto size = std::to_string(iop::MD5Hash::size);

    const auto printable = iop::scapeNonPrintable(iop::to_view(hashed));
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      iop_panic(sizeErr.toStdString() + hashed.c_str() + iop::StaticString(F(", expected size: ")).toStdString() + size);
    case iop::ParseError::NON_PRINTABLE:
      iop_panic(printErr.toStdString() + printable.borrow().begin());
    }
    iop_panic(iop::StaticString(F("Unexpected ParseError: ")).toStdString() + std::to_string(static_cast<uint8_t>(ref)));
  }
  hash.emplace(std::move(iop::unwrap_ok_mut(res, IOP_CTX())));
  return iop::unwrap_mut(hash, IOP_CTX());
}
iop::MacAddress & Device::macAddress() const noexcept {
  IOP_TRACE();
  static std::optional<iop::MacAddress> mac;
  if (mac.has_value())
    return iop::unwrap_mut(mac, IOP_CTX());

  constexpr const uint8_t macSize = 6;

  std::array<uint8_t, macSize> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  auto mac_ = iop::MacAddress::empty();
  const iop::StaticString fmtStr(F("%02X:%02X:%02X:%02X:%02X:%02X"));
  const auto *fmt = fmtStr.asCharPtr();
  // NOLINTNEXTLINE hicpp-vararg
  sprintf_P(reinterpret_cast<char *>(mac_.mutPtr()), fmt, buff[0], buff[1],
            buff[2], buff[3], buff[4], buff[5]); // NOLINT *-magic-numbers

  mac.emplace(std::move(mac_));
  return iop::unwrap_mut(mac, IOP_CTX());
}
}

#endif