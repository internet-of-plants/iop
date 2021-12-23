#ifndef IOP_DRIVER_STRING_STATIC_HPP
#define IOP_DRIVER_STRING_STATIC_HPP

#include <string_view>
#include <variant>
#include <array>
#include <string>
#include <cstring>

class __FlashStringHelper;

#ifdef IOP_DESKTOP
#define PROGMEM
#define PGM_P const char *
#define strlen_P strlen
#define memcpy_P memcpy
#define FLASH(string_literal) reinterpret_cast<const __FlashStringHelper *>(string_literal)
#else
#include <pgmspace.h>
#define FLASH(string_literal) reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal))
class String;
#endif

namespace iop {
/// Allows for possibly destructive operations, like scaping non printable characters
///
/// If the operation needs to modify the data it converts the `std::string_view` into the `std::string` variant
/// Otherwise it can keep storing the reference
using CowString = std::variant<std::string_view, std::string>;

#ifndef IOP_DESKTOP
auto to_view(const String& str) -> std::string_view;
#endif

auto to_view(const std::string& str) -> std::string_view;
auto to_view(const CowString& str) -> std::string_view;
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str, const size_t size) -> std::string_view {
  return std::string_view(str.data(), size);
}
template <size_t SIZE>
auto to_view(const std::array<char, SIZE>& str) -> std::string_view {
  return to_view(str, strnlen(str.begin(), str.max_size()));
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<std::array<char, SIZE>> &str, const size_t size) -> std::string_view {
  return to_view(str.get(), size);
}
template <size_t SIZE>
auto to_view(const std::reference_wrapper<std::array<char, SIZE>> &str) -> std::string_view {
  return to_view(str.get());
}
}

#endif