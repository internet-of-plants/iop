#ifndef IOP_STRING_VIEW_H_
#define IOP_STRING_VIEW_H_

#include <WString.h>
#include <static_string.hpp>

class StringView {
private:
  const char * str;

public:
  constexpr StringView(const char * string): str(string) {}
  StringView(const String& other): str(other.c_str()) {}
  constexpr StringView(const StaticString& other): str(other.get()) {}
  constexpr StringView(const StringView& other): str(other.str) {}
  void operator=(const StringView& other) { this->str = other.str; }
  void operator=(const StringView&& other) { this->str = other.str; }
  constexpr const char * const get() const { return reinterpret_cast<const char*>(this->str); }
  constexpr const char * const operator->() const { return this->get(); }
  constexpr const char * const operator*() const { return this->get(); }
  size_t length() const { return strlen(this->str); }
  bool isEmpty() const { return this->length() == 0; }
};

#endif