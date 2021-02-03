#include "utils.hpp"

#include "Esp.cpp"
#include "fixed_string.hpp"
#include "log.hpp"
#include "models.hpp"
#include "option.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "tracer.hpp"

static volatile std::array<InterruptEvent, interruptVariants> interruptEvents =
    {InterruptEvent::NONE};

namespace utils {
auto descheduleInterrupt() noexcept -> InterruptEvent {
  IOP_TRACE();
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
  IOP_TRACE();
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
  IOP_TRACE();
  static Option<MacAddress> mac;
  if (mac.isSome())
    return UNWRAP_REF(mac);

  constexpr const uint8_t macSize = 6;

  std::array<uint8_t, macSize> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data()); // NOLINT hicpp-no-array-decay

  auto mac_ = MacAddress::empty();
  PROGMEM_STRING(fmtStr, "%02X:%02X:%02X:%02X:%02X:%02X");
  const auto *fmt = fmtStr.asCharPtr();
  // NOLINTNEXTLINE *-magic-numbers *-pro-bounds-array-to-pointer-decay
  sprintf_P(reinterpret_cast<char *>(mac_.mutPtr()), fmt, buff[0], buff[1],
            buff[2], buff[3], buff[4], buff[5]); // NOLINT *-magic-numbers

  mac = std::move(mac_);
  return UNWRAP_REF(mac);
}

auto hashSketch() noexcept -> MD5Hash {
  IOP_TRACE();
  static Option<MD5Hash> result;
  if (result.isSome())
    return UNWRAP_REF(result);

  const auto hash = ESP.getSketchMD5();
  result = UNWRAP_OK(MD5Hash::fromString(UnsafeRawString(hash.c_str())));
  return UNWRAP_REF(result);
}
void logMemory(const Log &logger) noexcept {
  IOP_TRACE();
  logger.debug(F("Memory: "), String(ESP.getFreeHeap()), F(" "),
               String(ESP.getFreeContStack()), F(" "),
               String(ESP.getFreeSketchSpace()));
}
auto isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126;
}
} // namespace utils