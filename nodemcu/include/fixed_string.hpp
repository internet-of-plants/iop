#ifndef IOP_FIXED_STRING_VIEW_H_
#define IOP_FIXED_STRING_VIEW_H_

#include <storage.hpp>

/// String of fixed size. Heap allocated, but has fixed size
template <uint16_t SIZE>
class FixedString {
public:
  constexpr const static uint16_t size = SIZE;
private:
  Storage<SIZE> str;
public:
  FixedString<SIZE>(Storage<SIZE> other) noexcept: str(other) {}
  FixedString<SIZE>(FixedString<SIZE>& other) noexcept: str(other.str) {}
  FixedString<SIZE>(FixedString<SIZE>&& other) noexcept: str(std::move(other.str)) {}
  FixedString<SIZE>& operator=(FixedString<SIZE>& other) noexcept {
    this->str = other;
    return *this;
  }
  FixedString<SIZE>& operator=(FixedString<SIZE>&& other) = delete;
  constexpr const char * const get() const noexcept { return this->str.asString().get(); }
  char * const asMut() noexcept { return reinterpret_cast<char*>(this->str.mutPtr()); }
  Storage<SIZE>& operator->() const noexcept { return this->str; }
  Storage<SIZE>& operator*() const noexcept { return this->str; }
  static FixedString<SIZE> empty() noexcept { return Storage<SIZE>::empty(); }
  StringView asView() noexcept { return UnsafeRawPtr(this->get()); }
  size_t length() const noexcept { return strlen(this->get()); }
  bool isEmpty() const noexcept { return this->length() == 0; }
  static Result<FixedString<SIZE>, enum ParseError> fromString(const StringView str) noexcept { return Storage<SIZE>::fromString(str); }
  static FixedString<SIZE> fromStringTruncating(const StringView str) noexcept { return Storage<SIZE>::fromStringTruncating(str); }
  Storage<SIZE> intoInner() const noexcept { return std::move(this->val); }
};

#endif
