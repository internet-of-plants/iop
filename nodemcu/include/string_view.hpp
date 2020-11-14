#ifndef IOP_STRING_VIEW_H_
#define IOP_STRING_VIEW_H_

#include <string>
#include <WString.h>
#include <static_string.hpp>
#include <unsafe_raw_string.hpp>

/// Borrow type to temporarily access all string abstractions as one
/// It's basically a `const char*` with glitter. It's typesafe and can't be NULL
/// Unless the user uses the class UsafeRawString to construct it
///
/// Since it doesn't own anything it shouldn't be stored around,
/// it will endup outliving the storage and causing a use-after-free
///
/// Use it heavily as a function parameter, instead of the plain char*, it's typesafe and
/// its constructor will implicitly copy the internal pointer of each string abstraction
/// (and maybe make an alternative with StaticString to specialize for PROCMEM access)
/// Ending up being more "generic" than char* while safer and more performant (no implict casting char* to String)
class StringView {
private:
  const char * str;
public:
  constexpr StringView(const UnsafeRawString& str): str(str.get()) {}
  StringView(const std::string& str): str(str.c_str()) {}
  StringView(const String& other): str(other.c_str()) {}
  constexpr StringView(const StaticString& other): str(other.asCharPtr()) {}
  constexpr StringView(const StringView& other): str(other.str) {}
  void operator=(const StringView& other) { this->str = other.str; }
  void operator=(const StringView&& other) { this->str = other.str; }
  constexpr const char * const get() const { return this->str; }
  constexpr const char * const operator->() const { return this->get(); }
  constexpr const char * const operator*() const { return this->get(); }
  size_t length() const { return strlen(this->str); }
  bool isEmpty() const { return this->length() == 0; }
};

#endif