#ifndef IOP_UNSAFE_RAW_STRING_VIEW_H_
#define IOP_UNSAFE_RAW_STRING_VIEW_H_

#include <string>
#include <WString.h>

/// Type-safe runtime string pointers, it's the user telling the system
/// That they should be trusted and this UnsafeRawString will not contain a NULL pointer
/// And will not outlive the internal storage (as StringView)
///
/// It's basically a helper class to construct a StringView from a char*
/// Without implicit conversion byting us later.
/// It has to be explicit and with a greppable ` Unsafe` in the declaration
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