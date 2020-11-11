#ifndef IOP_STATIC_STRING_H_
#define IOP_STATIC_STRING_H_

#include <WString.h>

class StaticString {
private:
  const __FlashStringHelper * str;

public:
  constexpr StaticString(const __FlashStringHelper * str): str(str) {}
  constexpr StaticString(const StaticString& other): str(other.str) {}
  void operator=(const StaticString& other) { this->str = other.str; }
  void operator=(const StaticString&& other) { this->str = other.str; }
  constexpr const char * const get() const { return (const char *) this->str; }
  constexpr const char * const operator->() const { return (const char *) this->get(); }
  constexpr const char * const operator*() const { return (const char *) this->get(); }
};

#define STATIC_STRING(msg) StaticString(F(msg))

#endif