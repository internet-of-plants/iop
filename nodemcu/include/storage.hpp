#ifndef IOP_STORAGE_H_
#define IOP_STORAGE_H_

#include <memory>
#include <cinttypes>

#include <result.hpp>
#include <string_view.hpp>

struct MovedOut { uint8_t dummy; };

enum ParseError {
  TOO_BIG
};

/// Fixed size storage. Heap allocated (std::shared_ptr) std::array that has an extra zeroed byte (perfect to also abstract strings)
/// We heap allocate to prevent expensive copies and because our stack is so little
/// It creates the perfect environment to copy bounded data around without dynamic storages
/// Implements a fromString and fromStringTruncating (properly handling missing 0 terminators)
/// And can be accessed as StringView by the asString method
///
/// Used by FixedString<SIZE> and our structures stored in flash (check models.hpp)
/// providing a out of the box way to access flash while being type-safe
///
/// It can be used with `TYPED_STORAGE` macro to create a typed heap allocated Storage<SIZE>
/// avoiding mixing different data that happens to have the same size
template <uint16_t SIZE>
class Storage {
  public:
    static constexpr const size_t size = SIZE;
    typedef std::array<uint8_t, SIZE + 1> InnerStorage;
  private:
    std::shared_ptr<InnerStorage> val;

    // This dangerous, since we don't check here if std::shared_ptr is moved-out,
    // the caller should handle that. It only exists to provide a safe and efficient tryFrom
    Storage<SIZE>(std::shared_ptr<InnerStorage> val) noexcept: val(std::move(val)){ *this->val->end() = 0; }
  public:
    Storage<SIZE>(const InnerStorage val) noexcept: val(std::make_shared<InnerStorage>(val)) { *this->val->end() = 0; }
    Result<Storage<SIZE>, MovedOut> tryFrom(std::shared_ptr<InnerStorage> val) noexcept {
      if (val.get() == nullptr) return (MovedOut) { .dummy = 0 };
      return Storage<SIZE>(std::move(val));
    }
    constexpr Storage<SIZE>(const Storage<SIZE>& other) noexcept: val(other.val) {}
    constexpr Storage<SIZE>(Storage<SIZE> && other) noexcept: val(other.val) {}
    Storage<SIZE>& operator=(const Storage<SIZE>& other) noexcept {
      this->val = other.val;
      return *this;
    }
    Storage<SIZE>& operator=(const Storage<SIZE>&& other) noexcept {
      this->val = other.val;
      return *this;
    } 
    constexpr const uint8_t * constPtr() const noexcept { return this->val->data(); }
    constexpr uint8_t * mutPtr() noexcept { return this->val->data(); }
    StringView asString() const noexcept { return UnsafeRawString((const char*) this->constPtr()); }
    std::shared_ptr<InnerStorage> intoInner() noexcept { return this->val; }
    constexpr static Storage<SIZE> empty() noexcept { return Storage<SIZE>((InnerStorage) {0}); }

    static Storage<SIZE> fromStringTruncating(const StringView str) noexcept {
      auto val = Storage<SIZE>::empty();
      const auto len = str.length();
      memcpy(val.mutPtr(), (const uint8_t*) str.get(), len < SIZE ? len : SIZE);
      return val;
    }

    static Result<Storage<SIZE>, enum ParseError> fromString(const StringView str) noexcept {
      if (str.length() > SIZE) return ParseError::TOO_BIG;
      return Storage<SIZE>::fromStringTruncating(str);
    }
  };

  /// Creates a typed Storage<size_>, check it's documentation and definition for a clear understanding
  /// Prevents the mixup of different data that happens to have the same size
  #define TYPED_STORAGE(name, size_) \
  class name##_class {\
  public:\
    static constexpr const size_t size = size_;\
    typedef Storage<size> InnerStorage;\
  private:\
    InnerStorage val;\
  \
  public:\
    name##_class(const InnerStorage val) noexcept: val(val) {}\
    name##_class(const name##_class& other) noexcept: val(other.val) {}\
    name##_class(name##_class && other) noexcept: val(other.val) {}\
    name##_class& operator=(const name##_class& other) noexcept { this->val = other.val; return *this; }\
    name##_class& operator=(const name##_class&& other) noexcept { this->val = other.val; return *this; }\
    const uint8_t * constPtr() const noexcept { return this->val.constPtr(); }\
    uint8_t * mutPtr() noexcept { return this->val.mutPtr(); }\
    StringView asString() const noexcept { return this->val.asString(); }\
    static name##_class empty() noexcept { return name##_class(InnerStorage::empty()); }\
    InnerStorage intoInner() noexcept { return std::move(this->val); }\
    static name##_class fromStringTruncating(const StringView str) noexcept { return InnerStorage::fromStringTruncating(str); }\
    static Result<name##_class, enum ParseError> fromString(const StringView str) noexcept {\
      return InnerStorage::fromString(str).mapOk<name##_class>([](const InnerStorage& storage) noexcept {\
        return name##_class(storage);\
      });\
    }\
  };\
  typedef name##_class name;

#endif