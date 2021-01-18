#ifndef IOP_FIXED_STRING_HPP
#define IOP_FIXED_STRING_HPP

#include "storage.hpp"
#include "string_view.hpp"

/// String of fixed size. Heap allocated, but has fixed size
template <uint16_t SIZE> class FixedString {
private:
  Storage<SIZE> str;

public:
  constexpr const static uint16_t size = SIZE;

  ~FixedString<SIZE>() = default;
  explicit FixedString<SIZE>(Storage<SIZE> other) noexcept : str(other) {}
  FixedString<SIZE>(FixedString<SIZE> const &other) noexcept = default;
  FixedString<SIZE>(FixedString<SIZE> &&other) noexcept
      : str(std::move(other.str)) {}
  auto operator=(FixedString<SIZE> const &other)
      -> FixedString<SIZE> & = default;
  auto operator=(FixedString<SIZE> &&other) noexcept -> FixedString<SIZE> & {
    this->str = other.asStorage();
    return *this;
  }
  constexpr auto get() const noexcept -> const char * {
    return this->str.asString().get();
  }
  auto asMut() noexcept -> char * {
    return reinterpret_cast<char *>(this->str.mutPtr());
  }
  auto asStorage() const noexcept -> Storage<SIZE> { return this->str; }
  static auto empty() noexcept -> FixedString<SIZE> {
    return FixedString<SIZE>(Storage<SIZE>::empty());
  }
  auto length() const noexcept -> size_t { return strlen(this->get()); }
  auto isEmpty() const noexcept -> bool { return this->length() == 0; }
  auto operator*() const noexcept -> StringView { return this->str.asString(); }
  auto operator->() const noexcept -> StringView {
    return this->str.asString();
  }
  static auto fromString(const StringView str) noexcept
      -> Result<FixedString<SIZE>, enum ParseError> {
        return Storage<SIZE>::fromString(str);
      }
  static auto fromStringTruncating(const StringView str) noexcept
      -> FixedString<SIZE> {
    return Storage<SIZE>::fromStringTruncating(str);
  }
  auto intoInner() const noexcept -> Storage<SIZE> {
    return std::move(this->val);
  }
};

#endif
