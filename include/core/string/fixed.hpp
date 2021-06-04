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

  void clear() noexcept {
    this->str.clear();
  }

  static auto empty() noexcept -> FixedString<SIZE> {
    IOP_TRACE();
    return FixedString<SIZE>(Storage<SIZE>::empty());
  }

  constexpr auto get() const noexcept -> const char * {
    return reinterpret_cast<const char *>(this->str.constPtr());
  }

  /// You can accidentally store a non-printable character that will go unnotice
  /// using this
  auto asMut() noexcept -> char * {
    return reinterpret_cast<char *>(this->str.mutPtr());
  }
  /// You can accidentally store a non-printable character that will go unnotice
  /// using this
  auto asStorage() const noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return this->str;
  }

  auto length() const noexcept -> size_t {
    //IOP_TRACE();
    return strlen(this->get());
  }
  auto isEmpty() const noexcept -> bool {
    IOP_TRACE();
    return this->length() == 0;
  }

  static auto fromStorage(Storage<SIZE> other) noexcept
      -> std::variant<FixedString<SIZE>, ParseError> {
    IOP_TRACE();

    auto val = FixedString<SIZE>(other);
    if (!val->isAllPrintable())
      return ParseError::NON_PRINTABLE;
    return val;
  }

  static auto fromString(const std::string_view str) noexcept
      -> std::variant<FixedString<SIZE>, ParseError> {
    IOP_TRACE();
    auto val = Storage<SIZE>::fromString(std::move(str));
    if (std::holds_alternative<Storage<SIZE>>(val))
      return FixedString<SIZE>(std::move(std::get<Storage<SIZE>>(val, IOP_CTX())));
    return std::get<ParseError>(val, IOP_CTX());
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
