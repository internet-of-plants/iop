#ifndef IOP_UNSAFE_RAW_STRING_VIEW_H
#define IOP_UNSAFE_RAW_STRING_VIEW_H

class StringView;

/// Type-safe runtime string pointers, it's the user telling the system
/// That they should be trusted and this UnsafeRawString will not contain a NULL
/// pointer And will not outlive the internal storage
///
/// It's basically a helper class to construct a StringView from a char*
/// Without implicit conversion byting us later.
/// It has to be explicit and with a greppable ` Unsafe` in the declaration
class UnsafeRawString {
private:
  const char *str;

public:
  constexpr UnsafeRawString(const char *str) noexcept : str(str) {}
  constexpr UnsafeRawString(const UnsafeRawString &str) noexcept
      : str(str.str) {}
  constexpr UnsafeRawString(const UnsafeRawString &&str) noexcept
      : str(str.str) {}
  UnsafeRawString &operator=(const UnsafeRawString &other) noexcept {
    this->str = other.str;
    return *this;
  }
  UnsafeRawString &operator=(const UnsafeRawString &&other) noexcept {
    this->str = other.str;
    return *this;
  }
  constexpr const char *const get() const noexcept { return this->str; }
  StringView operator*() const noexcept;
  StringView operator->() const noexcept;
};

#endif