#ifndef IOP_UNSAFE_RAW_STRING_HPP
#define IOP_UNSAFE_RAW_STRING_HPP

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
  ~UnsafeRawString() = default;
  constexpr explicit UnsafeRawString(const char *str) noexcept : str(str) {}
  constexpr UnsafeRawString(const UnsafeRawString &str) noexcept = default;
  constexpr UnsafeRawString(UnsafeRawString &&str) noexcept : str(str.str) {}
  auto operator=(UnsafeRawString const &other) noexcept
      -> UnsafeRawString & = default;
  auto operator=(UnsafeRawString &&other) noexcept -> UnsafeRawString & {
    this->str = other.str;
    return *this;
  }
  constexpr auto get() const noexcept -> const char * { return this->str; }
  auto operator*() const noexcept -> StringView;
  auto operator->() const noexcept -> StringView;
};

#endif