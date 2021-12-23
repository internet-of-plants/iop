#ifndef IOP_CORE_STRING_STATIC_HPP
#define IOP_CORE_STRING_STATIC_HPP

#include "driver/string.hpp"

namespace iop {
/// Applies FNV hasing to convert a string to a `uint64_t`
auto hashString(const std::string_view txt) noexcept -> uint64_t;

/// Checks if a character is ASCII
auto isPrintable(const char ch) noexcept -> bool;

/// Checks if all characters are ASCII
auto isAllPrintable(const std::string_view txt) noexcept -> bool;

/// If some character isn't ASCII it returns a `std::string` based `CowString` that scapes them
/// If all characters are ASCII it returns a `std::string_view` based `CowString`
///
/// They are scaped by converting the `char` to `uint8_t` and then to string, and surrounding it by `<\` and `>`
///
/// For example the code: `iop::scapeNonPrintable("ABC \1")` returns `std::string("ABC <\1>")`
auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString;

using MD5Hash = std::array<char, 32>;
using MacAddress = std::array<char, 17>;

/// Helper string that holds a pointer to a string stored in PROGMEM
///
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid
/// defaulting to the String(__FlashStringHelper*) constructor that allocates implicitly.
///
/// It is not compatible with other strings because it requires 32 bits aligned reads, and
/// things like `strlen` will cause a crash, use `strlen_P`. So we separate them.
///
/// Either call a `_P` functions using `.getCharPtr()` to get the regular
/// pointer. Or construct a String with it using `.get()` so it knows to read
/// from PROGMEM.
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  StaticString() noexcept: str(nullptr) { this->str = FLASH(""); }
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept: str(str) {}
  
  /// Creates a `std::string` from the compile time string
  auto toString() const noexcept -> std::string;

  // Be careful when calling this function, if you pass PGM_P to a function that
  // expects a regular char* a hardware exception _may_ happen, PROGMEM data
  // needs to be read with 32 bits alignment, this has caused trouble in the
  // past and may do again.
  auto asCharPtr() const noexcept -> PGM_P { return reinterpret_cast<PGM_P>(this->get()); }

  auto get() const noexcept -> const __FlashStringHelper * { return this->str; }

  auto length() const noexcept -> size_t { return strlen_P(this->asCharPtr()); }
};

} // namespace iop

#endif