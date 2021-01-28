#ifndef IOP_STATIC_STRING_HPP
#define IOP_STATIC_STRING_HPP

#include "certificate_storage.hpp"

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
  ~StaticString();

  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept;
  StaticString(StaticString const &other) noexcept;
  StaticString(StaticString &&other) noexcept;
  auto operator=(StaticString const &other) noexcept -> StaticString &;
  auto operator=(StaticString &&other) noexcept -> StaticString &;
  auto get() const noexcept -> const __FlashStringHelper *;
  auto contains(const StringView needle) const noexcept -> bool;
  auto contains(const StaticString needle) const noexcept -> bool;
  auto length() const noexcept -> size_t;
  auto isEmpty() const noexcept -> bool;
  // Be careful when calling this function, if you pass PGM_P to a function that
  // expects a regular char* a hardware exception may happen, PROGMEM data needs
  // to be read in 32 bits alignment, this has caused trouble in the past and
  // may still do. It's why StringView doesn't have a constructor for
  // StaticString
  auto asCharPtr() const noexcept -> PGM_P;
};

#define PROGMEM_STRING(name, msg)                                              \
  static const char *const PROGMEM name##_progmem_char = msg;                  \
  static const StaticString name(                                              \
      FPSTR(name##_progmem_char)); // NOLINT cert-err58-cpp

#endif