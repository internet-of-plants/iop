#ifndef IOP_STORAGE_HPP
#define IOP_STORAGE_HPP

#include <cinttypes>
#include <memory>

#include "result.hpp"
#include "string_view.hpp"
#include "utils.hpp"

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
  typedef std::array<uint8_t, SIZE + 1> InnerStorage;

private:
  std::shared_ptr<InnerStorage> val;

  // This dangerous, since we don't check here if std::shared_ptr is moved-out,
  // the caller should handle that. It only exists to provide a safe and
  // efficient tryFrom
  explicit Storage<SIZE>(std::shared_ptr<InnerStorage> val) noexcept
      : val(std::move(val)) {
    *this->val->end() = 0;
  }

public:
  ~Storage<SIZE>() = default;
  explicit Storage<SIZE>(const InnerStorage val) noexcept
      : val(try_make_shared<InnerStorage>(val)) {
    if (!this->val)
      panic_(F("Unnable to allocate val"));

    *this->val->end() = 0;
  }
  auto tryFrom(std::shared_ptr<InnerStorage> val) noexcept
      -> Result<Storage<SIZE>, MovedOut> {
    if (val.get() == nullptr)
      return (MovedOut){.dummy = 0};
    return Storage<SIZE>(std::move(val));
  }
  constexpr Storage<SIZE>(Storage<SIZE> const &other) noexcept
      : val(other.val) {}
  constexpr Storage<SIZE>(Storage<SIZE> &&other) noexcept : val(other.val) {}
  auto operator=(Storage<SIZE> const &other) noexcept
      -> Storage<SIZE> & = default;
  auto operator=(Storage<SIZE> &&other) noexcept -> Storage<SIZE> & {
    this->val = other.val;
    return *this;
  }
  constexpr auto constPtr() const noexcept -> const uint8_t * {
    return this->val->data();
  }
  constexpr auto mutPtr() noexcept -> uint8_t * { return this->val->data(); }
  auto asString() const noexcept -> StringView {
    return UnsafeRawString((const char *)this->constPtr());
  }
  auto asSharedArray() const noexcept -> std::shared_ptr<InnerStorage> {
    return this->val;
  }
  constexpr static auto empty() noexcept -> Storage<SIZE> {
    return Storage<SIZE>((InnerStorage){0});
  }

  static auto fromStringTruncating(const StringView str) noexcept
      -> Storage<SIZE> {
    auto val = Storage<SIZE>::empty();
    const auto len = str.length();
    memcpy(val.mutPtr(), (const uint8_t *)str.get(), len < SIZE ? len : SIZE);
    return val;
  }

  static auto fromString(const StringView str) noexcept
      -> Result<Storage<SIZE>, enum ParseError> {
        if (str.length() > SIZE) return ParseError::TOO_BIG;
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
    typedef Storage<size> InnerStorage;                                        \
                                                                               \
  private:                                                                     \
    InnerStorage val;                                                          \
                                                                               \
  public:                                                                      \
    ~name##_class() = default;                                                 \
    explicit name##_class(InnerStorage val) noexcept : val(std::move(val)) {}  \
    name##_class(name##_class const &other) noexcept = default;                \
    name##_class(name##_class &&other) noexcept = default;                     \
    auto operator=(name##_class const &other) noexcept -> name                 \
        ##_class & = default;                                                  \
    auto operator=(name##_class &&other) noexcept -> name##_class & = default; \
    auto constPtr() const noexcept -> const uint8_t * {                        \
      return this->val.constPtr();                                             \
    }                                                                          \
    auto mutPtr() noexcept -> uint8_t * { return this->val.mutPtr(); }         \
    auto asString() const noexcept -> StringView {                             \
      return this->val.asString();                                             \
    }                                                                          \
    static auto empty() noexcept -> name##_class {                             \
      return name##_class(InnerStorage::empty());                              \
    }                                                                          \
    auto asSharedArray() const noexcept                                        \
        -> std::shared_ptr<InnerStorage::InnerStorage> {                       \
      return this->val.asSharedArray();                                        \
    }                                                                          \
    static auto fromStringTruncating(const StringView str) noexcept -> name    \
        ##_class {                                                             \
      return name##_class(InnerStorage::fromStringTruncating(str));            \
    }                                                                          \
    static auto fromString(const StringView str) noexcept                      \
        -> Result<name##_class, enum ParseError> {                             \
          return RESULT_MAP_OK(InnerStorage::fromString(str), name##_class,    \
                               [](InnerStorage storage) noexcept {             \
                                 return name##_class(std::move(storage));      \
                               });                                             \
        }                                                                      \
  };                                                                           \
  typedef name##_class name;

#endif