#ifdef IOP_DESKTOP
#include "driver/desktop/string.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/string.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/string.hpp"
#else
#error "Target not supported"
#endif

#include "driver/panic.hpp"

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

auto to_view(const std::string& str) -> std::string_view {
  return str.c_str();
}

auto to_view(const CowString& str) -> std::string_view {
  if (const auto *value = std::get_if<std::string_view>(&str)) {
    return *value;
  } else if (const auto *value = std::get_if<std::string>(&str)) {
    return iop::to_view(*value);
  }
  iop_panic(FLASH("Invalid variant types"));
}
} // namespace iop
