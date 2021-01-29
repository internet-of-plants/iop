#ifndef IOP_FIXED_STRING_HPP
#define IOP_FIXED_STRING_HPP

#include "certificate_storage.hpp"

#include "storage.hpp"
#include "string_view.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

/// String of fixed size. Heap allocated, but has fixed size
template <uint16_t SIZE> class FixedString {
private:
  Storage<SIZE> str;

public:
  constexpr const static uint16_t size = SIZE;

  ~FixedString<SIZE>() { IOP_TRACE(); }
  explicit FixedString<SIZE>(Storage<SIZE> other) noexcept : str(other) {
    IOP_TRACE();
  }
  FixedString<SIZE>(FixedString<SIZE> const &other) noexcept : str(other.str) {
    IOP_TRACE();
  }
  FixedString<SIZE>(FixedString<SIZE> &&other) noexcept
      : str(std::move(other.str)) {
    IOP_TRACE();
  }
  // NOLINTNEXTLINE cert-oop54-cpp
  auto operator=(FixedString<SIZE> const &other) -> FixedString<SIZE> & {
    IOP_TRACE();
    this->str = other.str;
    return *this;
  }
  auto operator=(FixedString<SIZE> &&other) noexcept -> FixedString<SIZE> & {
    IOP_TRACE();
    if (this == &other)
      return *this;
    this->str = std::move(other.str);
    return *this;
  }
  constexpr auto get() const noexcept -> const char * {
    IOP_TRACE();
    return this->str.asString().get();
  }
  auto asMut() noexcept -> char * {
    IOP_TRACE();
    return reinterpret_cast<char *>(this->str.mutPtr());
  }
  auto asStorage() const noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return this->str;
  }
  static auto empty() noexcept -> FixedString<SIZE> {
    IOP_TRACE();
    return FixedString<SIZE>(Storage<SIZE>::empty());
  }
  auto length() const noexcept -> size_t {
    IOP_TRACE();
    return strlen(this->get());
  }
  auto isEmpty() const noexcept -> bool {
    IOP_TRACE();
    return this->length() == 0;
  }
  auto operator*() const noexcept -> StringView {
    IOP_TRACE();
    return this->str.asString();
  }
  auto operator->() const noexcept -> StringView {
    IOP_TRACE();
    return this->str.asString();
  }
  // NOLINTNEXTLINE performance-unnecessary-value-param
  static auto fromString(const StringView str) noexcept
      -> Result<FixedString<SIZE>, enum ParseError> {
        IOP_TRACE(); return FixedString<SIZE>(Storage<SIZE>::fromString(str));
      }
  // NOLINTNEXTLINE performance-unnecessary-value-param
  static auto fromStringTruncating(const StringView str) noexcept
      -> FixedString<SIZE> {
    IOP_TRACE();
    return FixedString<SIZE>(Storage<SIZE>::fromStringTruncating(str));
  }
  auto intoInner() const noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return std::move(this->val);
  }
};

#endif
