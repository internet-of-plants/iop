#include "core/utils.hpp"
#include <optional>

#include "core/string/fixed.hpp"
#include "driver/device.hpp"

namespace iop {
auto hashString(const std::string_view txt) noexcept -> uint64_t {
  IOP_TRACE();
  const auto *const bytes = txt.begin();
  const uint64_t p = 16777619; // NOLINT cppcoreguidelines-avoid-magic-numbers
  uint64_t hash = 2166136261;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  const auto length = txt.length();
  for (uint32_t i = 0; i < length; ++i) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    hash = (hash ^ (uint64_t)bytes[i]) * p;
  }

  hash += hash << 13; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 7;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 3;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 17; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 5;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  return hash;
}

auto isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126; // NOLINT cppcoreguidelines-avoid-magic-numbers
}

auto isAllPrintable(const std::string_view txt) noexcept -> bool {
  const auto len = txt.length();
  for (uint8_t index = 0; index < len; ++index) {
    const auto ch = txt.begin()[index]; // NOLINT *-pro-bounds-pointer-arithmetic

    if (!isPrintable(ch))
      return false;
  }
  return true;
}

auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString {
  if (isAllPrintable(txt))
    return CowString(txt);

  const size_t len = txt.length();
  std::string s(len, '\0');
  for (uint8_t index = 0; index < len; ++index) {
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    const auto ch = txt.begin()[index];
    if (isPrintable(ch)) {
      s += ch;
    } else {
      s += "<\\";
      s += std::to_string(static_cast<uint8_t>(ch));
      s += ">";
    }
  }
  return CowString(s);
}

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

auto to_view(const std::string& str) -> std::string_view {
  return str.c_str();
}
#ifndef IOP_DESKTOP 
auto to_view(const String& str) -> std::string_view {
  return str.c_str();
}
#endif
auto to_view(const CowString& str) -> std::string_view {
  return str.borrow();
}
template <uint16_t SIZE>
auto to_view(const FixedString<SIZE>& str) -> std::string_view {
  return std::string_view(str.get());
}

auto hashSketch() noexcept -> const MD5Hash & {
  IOP_TRACE();
  static std::optional<MD5Hash> hash;
  if (hash.has_value())
    return iop::unwrap_ref(hash, IOP_CTX());

  // We could reimplement the internal function to avoid using String, but the
  // type safety and static cache are enough to avoid this complexity
  const std::string hashed(ESP.getSketchMD5().c_str());
  auto res = MD5Hash::fromString(hashed);
  if (iop::is_err(res)) {
    const auto &ref = iop::unwrap_err_ref(res, IOP_CTX());
    const StaticString sizeErr(F("MD5 hex is too big, this is critical: "));
    const StaticString printErr(F("Unprintable char in MD5 hex, this is critical: "));
    const auto size = std::to_string(MD5Hash::size);

    const auto printable = iop::scapeNonPrintable(hashed);
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      iop_panic(sizeErr.toStdString() + hashed + StaticString(F(", expected size: ")).toStdString() + size);
    case iop::ParseError::NON_PRINTABLE:
      iop_panic(printErr.toStdString() + printable.borrow().begin());
    }
    iop_panic(StaticString(F("Unexpected ParseError: ")).toStdString() + std::to_string(static_cast<uint8_t>(ref)));
  }
  hash.emplace(std::move(iop::unwrap_ok_mut(res, IOP_CTX())));
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