#include "utils.hpp"
#include "Esp.cpp"
#include "fixed_string.hpp"
#include "models.hpp"

static volatile std::array<InterruptEvent, interruptVariants> interruptEvents =
    {InterruptEvent::NONE};

namespace utils {
auto descheduleInterrupt() noexcept -> InterruptEvent {
  for (auto &el : interruptEvents._M_elems) {
    if (el != InterruptEvent::NONE) {
      const InterruptEvent tmp = el;
      el = InterruptEvent::NONE;
      return tmp;
    }
  }
  return InterruptEvent::NONE;
}
void scheduleInterrupt(const InterruptEvent ev) noexcept {
  Option<std::reference_wrapper<volatile InterruptEvent>> firstEmpty;
  for (auto &el : interruptEvents._M_elems) {
    if (el == InterruptEvent::NONE && firstEmpty.isNone())
      firstEmpty = std::ref(el);

    // We only allow one of each interrupt concurrently
    // So if we find ourselves we bail
    if (el == ev)
      return;
  }

  if (firstEmpty.isSome()) {
    UNWRAP(firstEmpty).get() = ev;
  } else {
    // Even with a bad interruptVariants it probably won't ever reach this
    panic_(F("Interrupt is not set, but there is no space available, please "
             "update interruptVariants to reflect the actual number of "
             "variants of InterruptEvent"));
  }
}

auto macAddress() noexcept -> MacAddress {
  static Option<MacAddress> mac;
  if (mac.isSome())
    return UNWRAP_REF(mac);

  constexpr const uint8_t macSize = 6;
  uint8_t buff[macSize];
  wifi_get_macaddr(STATION_IF, buff); // NOLINT hicpp-no-array-decay

  constexpr const uint8_t macStrSize = 18;
  char macStr[macStrSize] = {0};
  PROGMEM_STRING(fmtStr, "%02X:%02X:%02X:%02X:%02X:%02X");
  const auto *fmt = fmtStr.asCharPtr();
  // NOLINTNEXTLINE *-avoid-magic-numbers *-pro-bounds-array-to-pointer-decay
  sprintf_P(macStr, fmt, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);

  // Let's unwrap to fail fast instead of running with a bad mac
  // NOLINTNEXTLINE hicpp-no-array-decay
  mac = UNWRAP_OK(MacAddress::fromString(UnsafeRawString(macStr)));
  return UNWRAP_REF(mac);
}

auto hashSketch() noexcept -> MD5Hash {
  static MD5Hash result = MD5Hash::empty();
  if (result.asString().length() > 0)
    return result;

  uint32_t lengthLeft = ESP.getSketchSize();
  constexpr const size_t bufSize = 512;
  FixedString<bufSize> buf = FixedString<bufSize>::empty();
  uint32_t offset = 0;
  MD5Builder md5 = {};
  md5.begin();
  while (lengthLeft > 0) {
    size_t readBytes = (lengthLeft < bufSize) ? lengthLeft : bufSize;
    // NOLINTNEXTLINE *-pro-type-reinterpret-cast
    if (!ESP.flashRead(offset, reinterpret_cast<uint32_t *>(buf.asMut()),
                       (readBytes + 3) & ~3)) { // NOLINT hicpp-signed-bitwise
      return result;
    }
    md5.add(buf.asStorage().constPtr(), readBytes);
    lengthLeft -= readBytes;
    offset += readBytes;
  }
  md5.calculate();
  md5.getChars(reinterpret_cast<char *>(result.mutPtr()));
  return result;
}
} // namespace utils