#include "utils.hpp"

#include "Esp.cpp"
#include "core/log.hpp"
#include "core/memory.hpp"
#include "core/string/cow.hpp"
#include "models.hpp"

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
  iop::Option<std::reference_wrapper<volatile InterruptEvent>> firstEmpty;
  for (volatile auto &el : interruptEvents) {
    if (el == InterruptEvent::NONE && firstEmpty.isNone())
      firstEmpty.emplace(std::ref(el));

    // We only allow one of each interrupt concurrently
    // So if we find ourselves we bail
    if (el == ev)
      return;
  }

  if (firstEmpty.isSome()) {
    UNWRAP(firstEmpty).get() = ev;
  } else {
    // Even with a bad interruptVariants it probably won't ever reach this
    iop_panic(F("Interrupt is not set, but there is no space available, please "
                "update interruptVariants to reflect the actual number of "
                "variants of InterruptEvent"));
  }
}

auto macAddress() noexcept -> const MacAddress & {
  IOP_TRACE();
  static iop::Option<MacAddress> mac;
  if (mac.isSome())
    return UNWRAP_REF(mac);

  constexpr const uint8_t macSize = 6;

  std::array<uint8_t, macSize> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  auto mac_ = MacAddress::empty();
  PROGMEM_STRING(fmtStr, "%02X:%02X:%02X:%02X:%02X:%02X");
  const auto *fmt = fmtStr.asCharPtr();
  // NOLINTNEXTLINE hicpp-vararg
  sprintf_P(reinterpret_cast<char *>(mac_.mutPtr()), fmt, buff[0], buff[1],
            buff[2], buff[3], buff[4], buff[5]); // NOLINT *-magic-numbers

  mac.emplace(std::move(mac_));
  return UNWRAP_REF(mac);
}

auto hashSketch() noexcept -> const MD5Hash & {
  IOP_TRACE();
  static iop::Option<MD5Hash> hash;
  if (hash.isSome())
    return UNWRAP_REF(hash);

  const auto hashed = ESP.getSketchMD5();
  auto res = MD5Hash::fromString(iop::UnsafeRawString(hashed.c_str()));
  if (IS_ERR(res)) {
    const auto &ref = UNWRAP_ERR_REF(res);
    PROGMEM_STRING(sizeErr, "MD5 hex is too big, this is critical: ");
    PROGMEM_STRING(printErr, "Unprintable char in MD5 hex, this is critical: ");
    const String size(MD5Hash::size);

    const auto printable = iop::StringView(hashed).scapeNonPrintable();
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      iop_panic(String(sizeErr.get()) + hashed + F(", expected size: ") + size);
    case iop::ParseError::NON_PRINTABLE:
      iop_panic(String(printErr.get()) + printable.borrow().get());
    }
    iop_panic(String(F("Unexpected ParseError: ")) + static_cast<uint8_t>(ref));
  }
  hash.emplace(UNWRAP_OK(res));
  return UNWRAP_REF(hash);
}

void logMemory(const iop::Log &logger) noexcept {
  IOP_TRACE();
  logger.debug(F("Memory: "), String(ESP.getFreeHeap()), F(" "),
               String(ESP.getFreeContStack()), F(" "),
               String(ESP.getFreeSketchSpace()));
}
} // namespace utils