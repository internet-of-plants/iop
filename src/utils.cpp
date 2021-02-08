#include "utils.hpp"

#include "Esp.cpp"
#include "copy_on_write.hpp"
#include "fixed_string.hpp"
#include "log.hpp"
#include "models.hpp"
#include "option.hpp"
#include "panic.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "tracer.hpp"

static volatile InterruptEvent interruptEvents[interruptVariants] = {
    InterruptEvent::NONE};

namespace utils {
auto descheduleInterrupt() noexcept -> InterruptEvent {
  IOP_TRACE();
  for (volatile auto &el : interruptEvents) {
    if (el != InterruptEvent::NONE) {
      const auto tmp = el;
      el = InterruptEvent::NONE;
      return tmp;
    }
  }
  return InterruptEvent::NONE;
}
void scheduleInterrupt(const InterruptEvent ev) noexcept {
  IOP_TRACE();
  Option<std::reference_wrapper<volatile InterruptEvent>> firstEmpty;
  for (volatile auto &el : interruptEvents) {
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

auto macAddress() noexcept -> const MacAddress & {
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

auto hashSketch() noexcept -> const MD5Hash & {
  IOP_TRACE();
  static Option<MD5Hash> hash;
  if (hash.isSome())
    return UNWRAP_REF(hash);

  const auto hashed = ESP.getSketchMD5();
  auto res = MD5Hash::fromString(UnsafeRawString(hashed.c_str()));
  if (IS_ERR(res)) {
    const auto &ref = UNWRAP_ERR_REF(res);
    PROGMEM_STRING(sizeErr, "MD5 hex is too big, this is critical: ");
    PROGMEM_STRING(printErr, "Unprintable char in MD5 hex, this is critical: ");
    const String size(MD5Hash::size);

    const auto printable = utils::scapeNonPrintable(hashed);
    switch (ref) {
    case ParseError::TOO_BIG:
      panic_(String(sizeErr.get()) + hashed + F(", expected size: ") + size);
    case ParseError::NON_PRINTABLE:
      panic_(String(printErr.get()) + printable.borrow().get());
    }
    panic_(String(F("Unexpected ParseError: ")) + static_cast<uint8_t>(ref));
  }
  hash = UNWRAP_OK(res);
  return UNWRAP_REF(hash);
}
void logMemory(const Log &logger) noexcept {
  IOP_TRACE();
  logger.debug(F("Memory: "), String(ESP.getFreeHeap()), F(" "),
               String(ESP.getFreeContStack()), F(" "),
               String(ESP.getFreeSketchSpace()));
}

auto isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126; // NOLINT cppcoreguidelines-avoid-magic-numbers
}

auto scapeNonPrintable(StringView msg) noexcept -> CowString {
  if (msg.isAllPrintable())
    return CowString(msg);

  String s;
  // Scaped characters may use more, but avoids the bulk of reallocs
  s.reserve(msg.length());
  const size_t len = msg.length();
  for (uint8_t index = 0; index < len; ++index) {
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    const auto ch = std::move(msg).get()[index];
    if (utils::isPrintable(ch)) {
      s += ch;
    } else {
      s += F("<\\");
      s += String(static_cast<uint8_t>(ch));
      s += F(">");
    }
  }
  return CowString(s);
}
} // namespace utils