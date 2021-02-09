#ifndef IOP_CORE_STRING_FIXED_HPP
#define IOP_CORE_STRING_FIXED_HPP

#include "core/storage.hpp"

namespace iop {

/// Heapallocated string with fixed max size
template <uint16_t SIZE> class FixedString {
private:
  Storage<SIZE> str;

  explicit FixedString<SIZE>(Storage<SIZE> other) : str(std::move(other)) {}

public:
  constexpr const static uint16_t size = SIZE;

  static auto empty() noexcept -> FixedString<SIZE> {
    IOP_TRACE();
    return FixedString<SIZE>(Storage<SIZE>::empty());
  }

  constexpr auto get() const noexcept -> const char * {
    IOP_TRACE();
    return this->str.asString().get();
  }

  // TODO: You can accidentally store a non-printable character that will go
  // unnotice using this
  auto asMut() noexcept -> char * {
    IOP_TRACE();
    return reinterpret_cast<char *>(this->str.mutPtr());
  }
  // TODO: You can accidentally store a non-printable character that will go
  // unnotice using this
  auto asStorage() const noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return this->str;
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

  static auto fromStorage(Storage<SIZE> other) noexcept
      -> Result<FixedString<SIZE>, ParseError> {
    IOP_TRACE();

    auto val = FixedString<SIZE>(other);
    if (!val.isAllPrintable())
      return ParseError::NON_PRINTABLE;
    return val;
  }

  static auto fromString(const StringView str) noexcept
      -> Result<FixedString<SIZE>, ParseError> {
    IOP_TRACE();
    auto val = Storage<SIZE>::fromString(std::move(str));
    if (IS_OK(val))
      return FixedString<SIZE>(UNWRAP_OK(val));
    return UNWRAP_ERR(val);
  }

  ~FixedString<SIZE>() noexcept { IOP_TRACE(); }
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
    this->str = std::move(other.str);
    return *this;
  }
  auto operator=(FixedString<SIZE> &&other) noexcept -> FixedString<SIZE> & {
    IOP_TRACE();
    if (this == &other)
      return *this;
    this->str = std::move(other.str);
    return *this;
  }
};
} // namespace iop

#endif
