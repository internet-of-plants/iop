#ifndef IOP_CORE_STORAGE_HPP
#define IOP_CORE_STORAGE_HPP

// Remove this eventually to make core self-contained
#include "configuration.h"

#include "core/memory.hpp"
#include "core/result.hpp"

namespace iop {
enum class ParseError { TOO_BIG, NON_PRINTABLE };

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

  void printTrace() const noexcept {
    if (logLevel > LogLevel::TRACE)
      return;
    Serial.print(F("Storage<"));
    Serial.print(SIZE);
    Serial.print(F(">["));
    Serial.print(this->val.use_count());
    Serial.print(F("]("));
    for (const uint8_t byte : *this->val) {
      const auto ch = static_cast<char>(byte);
      if (StringView::isPrintable(ch)) {
        Serial.print(ch);
      } else {
        Serial.print(F("<\\"));
        Serial.print(byte);
        Serial.print(F(">"));
      }
    }
    Serial.println(F(")"));
    Serial.flush();
  }

public:
  ~Storage<SIZE>() noexcept {
    IOP_TRACE();
    this->printTrace();
  }
  explicit Storage<SIZE>(const InnerStorage val) noexcept
      : val(iop::try_make_shared<InnerStorage>(val)) {
    IOP_TRACE();
    if (!this->val)
      iop_panic(F("Unnable to allocate val"));

    *this->val->end() = 0;

    this->printTrace();
  }
  Storage<SIZE>(Storage<SIZE> const &other) noexcept : val(other.val) {
    IOP_TRACE();
    this->printTrace();
  }
  // NOLINTNEXTLINE cert-oop11-cpp
  Storage<SIZE>(Storage<SIZE> &&other) noexcept : val(other.val) {
    IOP_TRACE();
    this->printTrace();
  }
  // NOLINTNEXTLINE cert-oop54-cpp
  auto operator=(Storage<SIZE> const &other) noexcept -> Storage<SIZE> & {
    if (this == &other)
      return *this;
    IOP_TRACE();
    this->printTrace();
    this->val = other.val;
    this->printTrace();
    return *this;
  }
  auto operator=(Storage<SIZE> &&other) noexcept -> Storage<SIZE> & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->printTrace();
    this->val = other.val;
    this->printTrace();
    return *this;
  }
  auto constPtr() const noexcept -> const uint8_t * {
    IOP_TRACE();
    this->printTrace();
    return this->val->data();
  }
  auto mutPtr() noexcept -> uint8_t * {
    IOP_TRACE();
    this->printTrace();
    return this->val->data();
  }
  auto asString() const noexcept -> StringView {
    IOP_TRACE();
    this->printTrace();
    return UnsafeRawString(reinterpret_cast<const char *>(this->val->data()));
  }
  auto asSharedArray() const noexcept -> std::shared_ptr<InnerStorage> {
    IOP_TRACE();
    this->printTrace();
    return this->val;
  }
  static auto empty() noexcept -> Storage<SIZE> {
    IOP_TRACE();
    return Storage<SIZE>((InnerStorage){0});
  }

  static auto fromBytesUnsafe(const uint8_t *const b, const size_t len) noexcept
      -> Storage<SIZE> {
    IOP_TRACE();
    auto val = Storage<SIZE>::empty();
    memcpy(val.mutPtr(), b, len);
    val.printTrace();

    return val;
  }
  static auto fromString(StringView str) noexcept
      -> iop::Result<Storage<SIZE>, ParseError> {
    IOP_TRACE();

    uint8_t len = 0;
    while (len < SIZE && str.get()[len] != 0) {
      len++;
    }

    if (len > SIZE)
      return ParseError::TOO_BIG;

    auto val = Storage<SIZE>::empty();
    strncpy(reinterpret_cast<char *>(val.mutPtr()), std::move(str).get(), len);
    val.printTrace();
    if (!val.asString().isAllPrintable())
      return ParseError::NON_PRINTABLE;
    return val;
  }
};
} // namespace iop

/// Creates a typed Storage<size_>, check it's documentation and definition for
/// a clear understanding Prevents the mixup of different data that happens to
/// have the same size
#define TYPED_STORAGE(name, size_)                                             \
  class name##_class {                                                         \
  public:                                                                      \
    static constexpr const size_t size = size_;                                \
    using InnerStorage = iop::Storage<size>;                                   \
                                                                               \
  private:                                                                     \
    InnerStorage val;                                                          \
                                                                               \
  public:                                                                      \
    ~name##_class() noexcept { IOP_TRACE(); };                                 \
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
    auto asString() const noexcept -> iop::StringView {                        \
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
    static auto fromBytesUnsafe(const uint8_t *const b,                        \
                                const size_t len) noexcept -> name##_class {   \
      return name##_class(InnerStorage::fromBytesUnsafe(b, len));              \
    }                                                                          \
    static auto fromString(iop::StringView str) noexcept                       \
        -> iop::Result<name##_class, iop::ParseError> {                        \
      IOP_TRACE();                                                             \
      auto val = InnerStorage::fromString(std::move(str));                     \
      if (IS_OK(val))                                                          \
        return name##_class(UNWRAP_OK(val));                                   \
      return UNWRAP_ERR(val);                                                  \
    }                                                                          \
  };                                                                           \
  using name = name##_class; // NOLINT bugprone-macro-parentheses

#endif