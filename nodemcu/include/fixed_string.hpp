#ifndef IOP_FIXED_STRING_VIEW_H_
#define IOP_FIXED_STRING_VIEW_H_

#include <storage.hpp>

/// Don't let it fool you, this is heap allocated, just fixed size
template <uint16_t SIZE>
class FixedString {
public:
  constexpr const static uint16_t size = SIZE;
private:
  Storage<SIZE> str;
public:
  FixedString<SIZE>(Storage<SIZE> other): str(other) {}
  FixedString<SIZE>(FixedString<SIZE>& other): str(other.str) {}
  FixedString<SIZE>(FixedString<SIZE>&& other): str(std::move(other.str)) {}
  void operator=(FixedString<SIZE>& other) { this->str = other; }
  void operator=(FixedString<SIZE>&& other) { this->str = std::move(other); }
  constexpr const char * const get() const { return this->str.asString().get(); }
  Storage<SIZE>& operator->() const { return this->str; }
  Storage<SIZE>& operator*() const { return this->str; }
  static FixedString<SIZE> empty() { return Storage<SIZE>::empty(); }
  StringView asView() { return UnsafeRawPtr(this->get()); }
  size_t length() const { return strlen(this->get()); }
  bool isEmpty() const { return this->length() == 0; }
  static FixedString<SIZE> fromStringTruncating(const StringView str) { return Storage<SIZE>::fromStringTruncating(str); }
  Storage<SIZE> intoInner() const { return std::move(this->val); }
};

#endif
