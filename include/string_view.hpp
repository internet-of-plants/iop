#ifndef IOP_STRING_VIEW_HPP
#define IOP_STRING_VIEW_HPP

#include "WString.h"
#include "static_string.hpp"
#include "unsafe_raw_string.hpp"
#include <string>

/// Readonly non-owning reference to string abstractions,
/// treating them all as one.
///
/// Since it doesn't own anything it shouldn't be stored around,
/// it will endup outliving the inner storage and causing a use-after-free.
///
/// Use it heavily as a function parameter that isn't stored, instead of the
/// plain char* or String&, it's typesafe, doesn't allocate and its constructor
/// will implicitly copy the internal pointer of each string abstraction.
class StringView {
private:
  const char *str;

public:
  ~StringView() = default;
  // NOLINTNEXTLINE hicpp-explicit-conversions
  constexpr StringView(const UnsafeRawString str) noexcept : str(str.get()) {}
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const std::string &str) noexcept : str(str.c_str()) {}
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const String &other) noexcept : str(other.c_str()) {}
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StringView(const StaticString other) noexcept : str(other.asCharPtr()) {}
  constexpr StringView(const StringView &other) noexcept = default;
  constexpr StringView(StringView &&other) noexcept : str(other.str) {}
  auto operator==(const StringView &other) const noexcept -> bool {
    return strcmp(this->str, other.str) != 0;
  }
  auto operator==(const StaticString &other) const noexcept -> bool {
    return strcmp(this->str, other.asCharPtr()) != 0;
  }
  auto operator=(StringView const &other) noexcept -> StringView & = default;
  auto operator=(StringView &&other) noexcept -> StringView & {
    this->str = other.str;
    return *this;
  }
  auto get() const noexcept -> const char * { return this->str; }
  auto length() const noexcept -> size_t { return strlen(this->get()); }
  auto isEmpty() const noexcept -> bool { return this->length() == 0; }

  auto contains(const StringView needle) const noexcept -> bool {
    return strstr(this->get(), needle.get()) != nullptr;
  }

  auto contains(const StaticString needle) const noexcept -> bool {
    return strstr_P(this->get(), needle.asCharPtr()) != nullptr;
  }

  // FNV hash
  auto hash() const noexcept -> uint64_t {
    const auto *const bytes = reinterpret_cast<const uint8_t *>(this->get());
    const uint64_t p = 16777619; // NOLINT cppcoreguidelines-avoid-magic-numbers
    uint64_t hash = 2166136261;  // NOLINT cppcoreguidelines-avoid-magic-numbers

    const auto length = this->length();
    for (uint32_t i = 0; i < length; ++i) {
      hash = (hash ^ bytes[i]) * p; // NOLINT *-pro-bounds-pointer-arithmetic
    }

    hash += hash << 13; // NOLINT cppcoreguidelines-avoid-magic-numbers
    hash ^= hash >> 7;  // NOLINT cppcoreguidelines-avoid-magic-numbers
    hash += hash << 3;  // NOLINT cppcoreguidelines-avoid-magic-numbers
    hash ^= hash >> 17; // NOLINT cppcoreguidelines-avoid-magic-numbers
    hash += hash << 5;  // NOLINT cppcoreguidelines-avoid-magic-numbers

    return hash;
  }
};

#endif