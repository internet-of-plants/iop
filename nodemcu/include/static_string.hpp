#ifndef IOP_STATIC_STRING_H_
#define IOP_STATIC_STRING_H_

#include <WString.h>

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