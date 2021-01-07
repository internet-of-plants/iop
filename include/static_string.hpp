#ifndef IOP_STATIC_STRING_H
#define IOP_STATIC_STRING_H

#include "WString.h"

class StringView;

/// Helper string that holds a pointer to a string stored in PROGMEM
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid
/// defaulting to String(__FlashStringHelper*) constructor and allocating
/// implicitly
///
/// In ESP8266 the PROGMEM can just be accessed like any other RAM (besides the
/// correct alignment) So we technically don't need it to be type safe. But it's
/// a safe storage for it. And it works very well with `StringView` because of
/// that
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  constexpr StaticString(const __FlashStringHelper *str) noexcept : str(str) {}
  constexpr StaticString(const StaticString &other) noexcept : str(other.str) {}
  constexpr StaticString(const StaticString &&other) noexcept
      : str(other.str) {}
  StaticString &operator=(const StaticString &other) noexcept {
    this->str = other.str;
    return *this;
  }
  StaticString &operator=(const StaticString &&other) noexcept {
    this->str = other.str;
    return *this;
  }
  constexpr const __FlashStringHelper *const get() const noexcept {
    return this->str;
  }
  bool contains(const StaticString needle) const noexcept;
  bool contains(const StringView needle) const noexcept;
  size_t length() const noexcept { return strlen_P(this->asCharPtr()); }
  bool isEmpty() const noexcept { return this->length() == 0; }
  constexpr const char *const asCharPtr() const noexcept {
    return (const char *)this->get();
  }
};

#define PROGMEM_STRING(name, msg)                                              \
  static const char *const PROGMEM name##_progmem_char = msg;                  \
  static const StaticString name(                                              \
      reinterpret_cast<const __FlashStringHelper *>(name##_progmem_char));

#endif