#ifndef IOP_STRING_VIEW_H
#define IOP_STRING_VIEW_H

#include "WString.h"
#include "static_string.hpp"
#include "unsafe_raw_string.hpp"
#include <string>

/// Borrow type to temporarily access all string abstractions as one
/// It's basically a `const char*` with glitter. It's typesafe and can't be NULL
/// Unless the user uses the class UsafeRawString to construct it and messes up
///
/// Since it doesn't own anything it shouldn't be stored around,
/// it will endup outliving the inner storage and causing a use-after-free
///
/// Use it heavily as a function parameter, instead of the plain char*, it's
/// typesafe and its constructor will implicitly copy the internal pointer of
/// each string abstraction (and maybe make an alternative with StaticString to
/// specialize for PROCMEM access) Ending up being more "generic" than char*
/// while safer and more performant
// (no implict casting of char* to String)
class StringView {
private:
  const char *str;

public:
  constexpr StringView(const UnsafeRawString &str) noexcept : str(str.get()) {}
  StringView(const std::string &str) noexcept : str(str.c_str()) {}
  StringView(const String &other) noexcept : str(other.c_str()) {}
  constexpr StringView(const StaticString &other) noexcept
      : str(other.asCharPtr()) {}
  constexpr StringView(const StringView &other) noexcept : str(other.str) {}
  constexpr StringView(StringView &&other) noexcept : str(other.str) {}
  bool operator==(const StringView &other) const noexcept {
    return strcmp(this->str, other.str);
  }
  bool operator==(const StaticString &other) const noexcept {
    return strcmp(this->str, other.asCharPtr());
  }
  StringView &operator=(const StringView &other) noexcept {
    this->str = other.str;
    return *this;
  }
  StringView &operator=(const StringView &&other) noexcept {
    this->str = other.str;
    return *this;
  }
  constexpr const char *const get() const noexcept { return this->str; }
  size_t length() const noexcept { return strlen(this->get()); }
  bool isEmpty() const noexcept { return this->length() == 0; }

  bool contains(const StringView needle) {
    return strstr(this->get(), needle.get()) != nullptr;
  }

  bool contains(const StaticString needle) {
    return strstr_P(this->get(), needle.asCharPtr()) != nullptr;
  }

  // FNV hash
  uint64_t hash() const noexcept {
    const auto bytes = reinterpret_cast<const uint8_t *>(this->get());
    const uint64_t p = 16777619;
    uint64_t hash = 2166136261;

    const auto length = this->length();
    for (uint32_t i = 0; i < length; ++i) {
      hash = (hash ^ bytes[i]) * p;
    }

    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    return hash;
  }
};

#endif