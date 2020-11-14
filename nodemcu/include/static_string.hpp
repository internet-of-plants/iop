#ifndef IOP_STATIC_STRING_H_
#define IOP_STATIC_STRING_H_

#include <WString.h>

/// Helper string to hold a char pointer to a string stored in PROGMEM
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid defaulting to
/// String(__FlashStringHelper*) constructor
///
/// In ESP8266 the PROGMEM can just be accessed like any other RAM (besides the correct alignment)
/// So we technically don't need it to be thread safe. But it's a safe storage for it.
/// And it works very well with `StringView` because of that
class StaticString {
private:
  const __FlashStringHelper * str;

public:
  constexpr StaticString(const __FlashStringHelper * str): str(FPSTR(str)) {}
  constexpr StaticString(const StaticString& other): str(other.str) {}
  void operator=(const StaticString& other) { this->str = other.str; }
  void operator=(StaticString&& other) { this->str = other.str; }
  constexpr const __FlashStringHelper * const get() const { return this->str; }
  constexpr const char * const asCharPtr() const { return (const char *) this->get(); }
  constexpr const __FlashStringHelper * const operator*() const { return this->get(); }
  size_t length() const { return strlen((const char*) this->get()); }
  bool isEmpty() const { return this->length() == 0; }
};

#define PROGMEM_STRING(name, msg) static const char * const PROGMEM name##_progmem_char = msg;\
  static const StaticString name(reinterpret_cast<const __FlashStringHelper *>(name##_progmem_char));

#endif