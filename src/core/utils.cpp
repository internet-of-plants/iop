#include "core/utils.hpp"
#include "Esp.cpp"
#include "core/string/cow.hpp"

namespace iop {
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

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
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
  logger.info(F("Free Heap: "), String(ESP.getFreeHeap()), F(", Free Stack: "),
              String(ESP.getFreeContStack()), F(", Free Stack Space: "),
              String(ESP.getFreeSketchSpace()), F(", Heap Fragmentation: "),
              String(ESP.getHeapFragmentation()), F(", Biggest Heap Block: "),
              String(ESP.getMaxFreeBlockSize()));
}
} // namespace iop