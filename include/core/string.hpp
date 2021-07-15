#ifndef IOP_CORE_STRING_STATIC_HPP
#define IOP_CORE_STRING_STATIC_HPP

#include <string_view>
#include <variant>
#include <array>
#include <string>
#include "driver/string.hpp"

namespace iop {
using CowString = std::variant<std::string_view, std::string>;

auto hashString(const std::string_view txt) noexcept -> uint64_t; // FNV hash
auto isPrintable(const char ch) noexcept -> bool;
auto isAllPrintable(const std::string_view txt) noexcept -> bool;
auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString;
#ifndef IOP_DESKTOP
auto to_view(const String& str) -> std::string_view;
#endif
auto to_view(const std::string& str) -> std::string_view;
auto to_view(const CowString& str) -> std::string_view;
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str) -> std::string_view {
  return std::string_view(str.data(), strnlen(str.begin(), str.max_size()));
}

/// Helper string that holds a pointer to a string stored in PROGMEM
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid
/// defaulting to String(__FlashStringHelper*) constructor and allocating
/// implicitly.
///
/// It is not compatible with other strings because it requires aligned reads, and
/// things like `strlen` violate it. So we separate.
///
/// Either call a `_P` functions using `.getCharPtr()` to get the regular
/// pointer. Or construct a String with it using `.get()` so it knows to read
/// from PROGMEM
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  StaticString() noexcept: str(nullptr) {
    this->str = F("");
  }
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept;

  auto get() const noexcept -> const __FlashStringHelper *;
  auto contains(std::string_view needle) const noexcept -> bool;
  auto contains(StaticString needle) const noexcept -> bool;
  auto length() const noexcept -> size_t;
  auto isEmpty() const noexcept -> bool;
  auto toStdString() const noexcept -> std::string;

  // Be careful when calling this function, if you pass PGM_P to a function that
  // expects a regular char* a hardware exception _may_ happen, PROGMEM data
  // needs to be read with 32 bits alignment, this has caused trouble in the
  // past and may do again. It's why it can't be converted to other strings
  auto asCharPtr() const noexcept -> PGM_P;

  ~StaticString() noexcept = default;
  StaticString(StaticString const &other) noexcept = default;
  StaticString(StaticString &&other) noexcept;
  auto operator=(StaticString const &other) noexcept
      -> StaticString & = default;
  auto operator=(StaticString &&other) noexcept -> StaticString & = default;
};

} // namespace iop

#endif