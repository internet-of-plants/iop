#include "core/utils.hpp"
#include <optional>

#ifdef IOP_DESKTOP
class Esp {
public:
  uint16_t getFreeHeap() { return 1000; }
  uint16_t getFreeContStack() { return 1000; }
  uint16_t getMaxFreeBlockSize() { return 1000; }
  uint16_t getHeapFragmentation() { return 0; }
  uint16_t getFreeSketchSpace() { return 1000; }
  std::string getSketchMD5() { return "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"; }
};
static Esp ESP;
#define STATION_IF 0
static void wifi_get_macaddr(uint8_t station, uint8_t *buff) {
  (void) station;
  memcpy(buff, "AA:AA:AA:AA:AA:AA", 18);
}
#define sprintf_P sprintf
#else
#include "Esp.cpp"
#include "user_interface.h"
#endif

namespace iop {
auto macAddress() noexcept -> const MacAddress & {
  IOP_TRACE();
  static std::optional<MacAddress> mac;
  if (mac.has_value())
    return iop::unwrap_ref(mac, IOP_CTX());

  constexpr const uint8_t macSize = 6;

  std::array<uint8_t, macSize> buff = {0};
  wifi_get_macaddr(STATION_IF, buff.data());

  auto mac_ = MacAddress::empty();
  const StaticString fmtStr(F("%02X:%02X:%02X:%02X:%02X:%02X"));
  const auto *fmt = fmtStr.asCharPtr();
  // NOLINTNEXTLINE hicpp-vararg
  sprintf_P(reinterpret_cast<char *>(mac_.mutPtr()), fmt, buff[0], buff[1],
            buff[2], buff[3], buff[4], buff[5]); // NOLINT *-magic-numbers

  mac.emplace(std::move(mac_));
  return iop::unwrap_ref(mac, IOP_CTX());
}

auto hashSketch() noexcept -> const MD5Hash & {
  IOP_TRACE();
  static std::optional<MD5Hash> hash;
  if (hash.has_value())
    return iop::unwrap_ref(hash, IOP_CTX());

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
  const std::string hashed(ESP.getSketchMD5().c_str());
  auto res = MD5Hash::fromString(iop::UnsafeRawString(hashed.c_str()));
  if (IS_ERR(res)) {
    const auto &ref = UNWRAP_ERR_REF(res);
    const StaticString sizeErr(F("MD5 hex is too big, this is critical: "));
    const StaticString printErr(F("Unprintable char in MD5 hex, this is critical: "));
    const auto size = std::to_string(MD5Hash::size);

    const auto printable = iop::StringView(hashed).scapeNonPrintable();
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      iop_panic(sizeErr.toStdString() + hashed + StaticString(F(", expected size: ")).toStdString() + size);
    case iop::ParseError::NON_PRINTABLE:
      iop_panic(printErr.toStdString() + printable.borrow().get());
    }
    iop_panic(StaticString(F("Unexpected ParseError: ")).toStdString() + std::to_string(static_cast<uint8_t>(ref)));
  }
  hash.emplace(UNWRAP_OK(res));
  return iop::unwrap_ref(hash, IOP_CTX());
}
void logMemory(const iop::Log &logger) noexcept {
  IOP_TRACE();
  logger.info(F("Free Heap: "), std::to_string(ESP.getFreeHeap()), F(", Free Stack: "),
              std::to_string(ESP.getFreeContStack()), F(", Free Stack Space: "),
              std::to_string(ESP.getFreeSketchSpace()), F(", Heap Fragmentation: "),
              std::to_string(ESP.getHeapFragmentation()), F(", Biggest Heap Block: "),
              std::to_string(ESP.getMaxFreeBlockSize()));
}
} // namespace iop