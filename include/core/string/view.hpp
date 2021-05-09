#ifndef IOP_CORE_STRING_VIEW_HPP
#define IOP_CORE_STRING_VIEW_HPP

#include "core/string/static.hpp"
#include "core/string/unsafe_raw.hpp"

#include <cstdint>
#include <string>

#define IOP_FILE iop::StaticString(FPSTR(__FILE__))
#define IOP_LINE (uint32_t) __LINE__
#define IOP_FUNC iop::StringView(iop::UnsafeRawString(__PRETTY_FUNCTION__))

namespace iop {
template <uint16_t SIZE> class FixedString;
class CowString;

/// Readonly non-owning reference to string abstractions,
/// handling them all with one unified interface.
///
/// Since it doesn't own anything it shouldn't be stored around,
/// it will endup outliving the inner storage and causing a use-after-free.
///
/// Use it heavily as a function parameter that isn't stored, instead of the
/// plain char* or String&, it's typesafe, doesn't allocate and its
/// constructor will implicitly copy the internal pointer of each string
/// abstraction available.
class StringView {
private:
  const char *str;

public:
  ~StringView() noexcept = default;
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const UnsafeRawString &str) noexcept : str(str.get()) {}
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const std::string &str) noexcept : str(str.c_str()) {}
  #ifndef IOP_DESKTOP
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const String &other) noexcept : str(other.c_str()) {}
  #endif
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const CowString &str) noexcept;
  template <uint16_t SIZE>
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const FixedString<SIZE> &other) noexcept : str(other.get()) {}
  StringView(const StringView &other) noexcept = default;
  StringView(StringView &&other) noexcept = default;
  auto operator==(const StringView &other) const noexcept -> bool;
  auto operator==(const StaticString &other) const noexcept -> bool;
  auto operator=(StringView const &other) noexcept -> StringView & = default;
  auto operator=(StringView &&other) noexcept -> StringView & = default;
  auto get() const noexcept -> const char *;
  auto length() const noexcept -> size_t;
  auto isEmpty() const noexcept -> bool;
  auto contains(StringView needle) const noexcept -> bool;
  auto contains(StaticString needle) const noexcept -> bool;
  auto indexOf(StringView needle) const noexcept -> ssize_t;
  auto indexOf(StaticString needle) const noexcept -> ssize_t;
  auto hash() const noexcept -> uint64_t; // FNV hash
  auto isAllPrintable() const noexcept -> bool;
  auto scapeNonPrintable() const noexcept -> CowString;
  static auto isPrintable(char ch) noexcept -> bool;
};

static const StringView emptyStringView = std::string();
} // namespace iop

#endif