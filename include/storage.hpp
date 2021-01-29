#ifndef IOP_STORAGE_HPP
#define IOP_STORAGE_HPP

#include "certificate_storage.hpp"

#include "configuration.h"

#include "result.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"
#include "utils.hpp"

#include <cinttypes>
#include <memory>

struct MovedOut {
  uint8_t dummy;
};

enum ParseError { TOO_BIG };

/// Fixed size storage. Heap allocated (std::shared_ptr) std::array that has an
/// extra zeroed byte (perfect to also abstract strings) We heap allocate to
/// prevent expensive copies and because our stack is so little It creates the
/// perfect environment to copy bounded data around without dynamic storages
/// Implements a fromString and fromStringTruncating (properly handling missing
/// 0 terminators) And can be accessed as StringView by the asString method
///
/// Used by FixedString<SIZE> and our structures stored in flash (check
/// models.hpp) providing a out of the box way to access flash while being
/// type-safe
///
/// It can be used with `TYPED_STORAGE` macro to create a typed heap allocated
/// Storage<SIZE> avoiding mixing different data that happens to have the same
/// size
template <uint16_t SIZE> class Storage {
public:
  static constexpr const size_t size = SIZE;
  using InnerStorage = std::array<uint8_t, SIZE + 1>;

private:
  std::shared_ptr<InnerStorage> val;

  // This dangerous, since we don't check here if std::shared_ptr is moved-out,
  // the caller should handle that. It only exists to provide a safe and
  // efficient tryFrom
  explicit Storage<SIZE>(std::shared_ptr<InnerStorage> val) noexcept
      : val(std::move(val)) {
    IOP_TRACE();
    *this->val->end() = 0;
  }

public:
  ~Storage<SIZE>() {
    IOP_TRACE();
    if (logLevel > LogLevel::TRACE)
      return;
    Serial.print(F("~Storage<"));
    Serial.print(SIZE);
    Serial.print(F(">["));
    Serial.print(this->val.use_count());
    Serial.print(F("]("));
    Serial.print(reinterpret_cast<char *>(this->val->data()));
    Serial.println(F(")"));
    Serial.flush();
  }
  explicit Storage<SIZE>(const InnerStorage val) noexcept
      : val(try_make_shared<InnerStorage>(val)) {
    IOP_TRACE();
    if (!this->val)
      panic_(F("Unnable to allocate val"));

    *this->val->end() = 0;
  }
  Storage<SIZE>(Storage<SIZE> const &other) noexcept : val(other.val) {
    IOP_TRACE();
  }
  // NOLINTNEXTLINE cert-oop11-cpp
  Storage<SIZE>(Storage<SIZE> &&other) noexcept : val(other.val) {
    IOP_TRACE();
  }
  // NOLINTNEXTLINE cert-oop54-cpp
  auto operator=(Storage<SIZE> const &other) noexcept -> Storage<SIZE> & {
    if (this == &other)
      return *this;

    IOP_TRACE();
    this->val = other.val;
    return *this;
  }
  // NOLINTNEXTLINE cert-oop11-cpp
  auto operator=(Storage<SIZE> &&other) noexcept -> Storage<SIZE> & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->val = other.val;
    return *this;
  }
  auto constPtr() const noexcept -> const uint8_t * {
    IOP_TRACE();
    return this->val->data();
  }
  auto mutPtr() noexcept -> uint8_t * {
    IOP_TRACE();
    return this->val->data();
  }
  auto asString() const noexcept -> StringView {
    IOP_TRACE();
    return UnsafeRawString((const char *)this->constPtr());
  }
  auto asSharedArray() const noexcept -> std::shared_ptr<InnerStorage> {
    IOP_TRACE();
    return this->val;
  }
  static auto empty() noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return Storage<SIZE>((InnerStorage){0});
  }

  // NOLINTNEXTLINE performance-unnecessary-value-param
  static auto fromStringTruncating(const StringView str) noexcept
      -> Storage<SIZE> {
    IOP_TRACE();
    auto val = Storage<SIZE>::empty();
    uint8_t len = 0;
    while (len < SIZE && str.get()[len++] != 0) {
    }
    strncpy(reinterpret_cast<char *>(val.mutPtr()), str.get(), len);
    return val;
  }
  // NOLINTNEXTLINE performance-unnecessary-value-param
  static auto fromString(const StringView str) noexcept
      -> Result<Storage<SIZE>, enum ParseError> {
        IOP_TRACE(); if (str.length() > SIZE) return ParseError::TOO_BIG;
        return Storage<SIZE>::fromStringTruncating(str);
      }
};

/// Creates a typed Storage<size_>, check it's documentation and definition for
/// a clear understanding Prevents the mixup of different data that happens to
/// have the same size
#define TYPED_STORAGE(name, size_)                                             \
  class name##_class {                                                         \
  public:                                                                      \
    static constexpr const size_t size = size_;                                \
    using InnerStorage = Storage<size>;                                        \
                                                                               \
  private:                                                                     \
    InnerStorage val;                                                          \
                                                                               \
  public:                                                                      \
    ~name##_class() { IOP_TRACE(); };                                          \
    explicit name##_class(InnerStorage val) noexcept : val(std::move(val)) {   \
      IOP_TRACE();                                                             \
    }                                                                          \
    name##_class(name##_class const &other) noexcept : val(other.val) {        \
      IOP_TRACE();                                                             \
    }                                                                          \
    name##_class(name##_class &&other) noexcept : val(std::move(other.val)) {  \
      IOP_TRACE();                                                             \
    };                                                                         \
    auto operator=(name##_class const &other) noexcept -> name##_class & {     \
      IOP_TRACE();                                                             \
      if (this == &other)                                                      \
        return *this;                                                          \
      this->val = other.val;                                                   \
      return *this;                                                            \
    }                                                                          \
    auto operator=(name##_class &&other) noexcept -> name##_class & {          \
      IOP_TRACE();                                                             \
      if (this == &other)                                                      \
        return *this;                                                          \
      this->val = std::move(other.val);                                        \
      return *this;                                                            \
    }                                                                          \
    auto constPtr() const noexcept -> const uint8_t * {                        \
      IOP_TRACE();                                                             \
      return this->val.constPtr();                                             \
    }                                                                          \
    auto mutPtr() noexcept -> uint8_t * {                                      \
      IOP_TRACE();                                                             \
      return this->val.mutPtr();                                               \
    }                                                                          \
    auto asString() const noexcept -> StringView {                             \
      IOP_TRACE();                                                             \
      return this->val.asString();                                             \
    }                                                                          \
    static auto empty() noexcept -> name##_class {                             \
      IOP_TRACE();                                                             \
      return name##_class(InnerStorage::empty());                              \
    }                                                                          \
    auto asSharedArray() const noexcept                                        \
        -> std::shared_ptr<InnerStorage::InnerStorage> {                       \
      IOP_TRACE();                                                             \
      return this->val.asSharedArray();                                        \
    }                                                                          \
    static auto fromStringTruncating(StringView str) noexcept -> name          \
        ##_class {                                                             \
      IOP_TRACE();                                                             \
      return name##_class(InnerStorage::fromStringTruncating(std::move(str))); \
    }                                                                          \
    static auto fromString(StringView str) noexcept                            \
        -> Result<name##_class, enum ParseError> {                             \
          IOP_TRACE(); auto inner = InnerStorage::fromString(std::move(str));  \
          if (IS_OK(inner)) return name##_class(UNWRAP_OK(inner));             \
          return UNWRAP_ERR(inner);                                            \
        }                                                                      \
  };                                                                           \
  using name = name##_class; // NOLINT bugprone-macro-parentheses

#endif