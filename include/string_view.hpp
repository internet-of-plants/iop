#ifndef IOP_STRING_VIEW_HPP
#define IOP_STRING_VIEW_HPP

#include "certificate_storage.hpp"

#include "static_string.hpp"
#include "tracer.hpp"
#include <string>

#include "WString.h"

class UnsafeRawString;
template <uint16_t SIZE> class FixedString;

/// Readonly non-owning reference to string abstractions,
/// treating them all as one.
///
/// Since it doesn't own anything it shouldn't be stored around,
/// it will endup outliving the inner storage and causing a use-after-free.
///
/// Use it heavily as a function parameter that isn't stored, instead of the
/// plain char* or String&, it's typesafe, doesn't allocate and its
/// constructor will implicitly copy the internal pointer of each string
/// abstraction.
class StringView {
private:
  const char *str;

public:
  ~StringView();
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const UnsafeRawString &str) noexcept;
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const std::string &str) noexcept;
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const String &other) noexcept;
  template <uint16_t SIZE>
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const FixedString<SIZE> &other) noexcept : str(other.get()) {
    // IOP_TRACER();
  }
  StringView(const StringView &other) noexcept = default;
  StringView(StringView &&other) noexcept = default;
  auto operator==(const StringView &other) const noexcept -> bool;
  auto operator==(const StaticString &other) const noexcept -> bool;
  auto operator=(StringView const &other) noexcept -> StringView & = default;
  auto operator=(StringView &&other) noexcept -> StringView & = default;
  auto get() const noexcept -> const char *;
  auto length() const noexcept -> size_t;
  auto isEmpty() const noexcept -> bool;
  // NOLINTNEXTLINE performance-unnecessary-value-param
  auto contains(const StringView needle) const noexcept -> bool;
  // NOLINTNEXTLINE performance-unnecessary-value-param
  auto contains(const StaticString needle) const noexcept -> bool;
  auto hash() const noexcept -> uint64_t; // FNV hash
};

#endif