#ifndef IOP_UNSAFE_RAW_STRING_VIEW_H_
#define IOP_UNSAFE_RAW_STRING_VIEW_H_

#include <string>
#include <WString.h>

class UnsafeRawString {
private:
  const char * str;
public:
  constexpr UnsafeRawString(const char* str): str(str) {}
  void operator=(const UnsafeRawString& other) { this->str = other.str; }
  void operator=(const UnsafeRawString&& other) { this->str = other.str; }
  constexpr const char * const get() const { return this->str; }
  constexpr const char * const operator*() const { return this->get(); }
  size_t length() const { return strlen(this->str); }
  bool isEmpty() const { return this->length() == 0; }
};

#endif