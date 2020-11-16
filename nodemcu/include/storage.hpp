#ifndef IOP_STORAGE_H_
#define IOP_STORAGE_H_

#include <panic.hpp>
#include <cstdint>
#include <string_view.hpp>

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

    void panicIfMovedOut() const {
      if (this->val.get() == nullptr) {
        panic_(F("Tried to use a storage that has been moved out"));
      }
    }
  public:
    Storage<SIZE>(const InnerStorage val): val(std::make_shared<InnerStorage>(val)) { *this->val->end() = 0; }
    Storage<SIZE>(std::shared_ptr<InnerStorage> val): val(std::move(val)) {
      // Just to make sure val doesn't come moved out by accident
      this->panicIfMovedOut();
      *this->val->end() = 0;
    }
    constexpr Storage<SIZE>(const Storage<SIZE>& other): val(other.val) {}
    // Hijacks moves so you can't invalidly use it (blame it on cpp move not being enforced by the compiler)
    Storage<SIZE>(Storage<SIZE> && other): val(other.val) {
      this->panicIfMovedOut();
    }
    void operator=(const Storage<SIZE>& other) { this->val = other.val; }
    void operator=(Storage<SIZE> && other) { this->val = std::move(other.val); }
    constexpr const uint8_t * constPtr() const { return this->val->data(); }
    constexpr uint8_t * mutPtr() { return this->val->data(); }
    constexpr StringView asString() const { return UnsafeRawString((const char *) this->val->data()); }
    std::shared_ptr<InnerStorage> intoInner() { return std::move(this->val); }
    constexpr static Storage<SIZE> empty() { return Storage<SIZE>((InnerStorage) {0}); }
    bool operator==(const Storage<SIZE>& other) const { return memcmp(this->constPtr(), other.constPtr(), SIZE) == 0; }
    bool operator!=(const Storage<SIZE>& other) const { return !this->operator==(other); }

    static Storage<SIZE> fromStringTruncating(const StringView str) {
      const uint16_t len = strnlen(str.get(), SIZE);
      auto val = std::make_shared<InnerStorage>();
      val->fill(0);
      memcpy(val->data(), (uint8_t*) str.get(), len);
      return Storage<SIZE>(val);
    }

    static Result<Storage<SIZE>, enum ParseError> fromString(const StringView str) {
      if (strnlen(str.get(), SIZE) == SIZE && str.get()[SIZE] != 0) {
        return ParseError::TOO_BIG;
      }
      auto val = std::make_shared<InnerStorage>();
      val->fill(0);
      memcpy(val.get(), str.get(), SIZE);
      return Storage<SIZE>(val);
    }
  };

  /// Creates a typed Storage<size_>, check it's documentation and definition for a clear understanding
  /// Prevents the mixup of different data that happens to have the same size
  #define TYPED_STORAGE(name, size_) \
  class name##_class {\
  public:\
    static constexpr const size_t SIZE = size_;\
    static constexpr const size_t size = SIZE + 1;\
    typedef Storage<size> InnerStorage;\
  private:\
    InnerStorage val;\
  \
  public:\
    name##_class(const InnerStorage val): val(val) {}\
    name##_class(const name##_class& other): val(other.val) {}\
    name##_class(name##_class && other): val(std::move(other.val)) {}\
    void operator=(const name##_class& other) { this->val = other.val; }\
    void operator=(name##_class && other) { this->val = std::move(other.val); }\
    bool operator==(const name##_class& other) const { return this->val == other.val; }\
    bool operator!=(const name##_class& other) const { return this->val != other.val; }\
    const uint8_t * constPtr() const { return this->val.constPtr(); }\
    uint8_t * mutPtr() { return this->val.mutPtr(); }\
    StringView asString() const { return this->val.asString(); }\
    static name##_class empty() { return name##_class(InnerStorage::empty()); }\
    InnerStorage intoInner() { return std::move(this->val); }\
    static name##_class fromStringTruncating(const StringView str) { return InnerStorage::fromStringTruncating(str); }\
    static Result<name##_class, enum ParseError> fromString(const StringView str) {\
      return InnerStorage::fromString(str).mapOk<name##_class>([](const InnerStorage& storage) {\
        return name##_class(storage);\
      });\
    }\
  };\
  typedef name##_class name;

  #endif
