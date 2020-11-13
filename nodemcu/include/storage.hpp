#ifndef IOP_STORAGE_H_
#define IOP_STORAGE_H_

#include <cstdint>
#include <string_view.hpp>

enum ParseError {
  TOO_BIG
};

/// This uses SIZE + 1 of memory (heap allocated as std::shared_ptr), it's 0 terminated by default
/// to allow to be used as string trivially
template <uint16_t SIZE>
class Storage {
  public:
    static constexpr const size_t size = SIZE;
    typedef std::array<uint8_t, SIZE + 1> InnerStorage;
  private:
    std::shared_ptr<InnerStorage> val;

  public:
    Storage<SIZE>(const InnerStorage val): val(std::make_shared<InnerStorage>(val)) { *this->val->end() = 0; }
    Storage<SIZE>(std::shared_ptr<InnerStorage> val): val(std::move(val)) { *this->val->end() = 0; }
    constexpr Storage<SIZE>(const Storage<SIZE>& other): val(other.val) {}
    constexpr Storage<SIZE>(Storage<SIZE> && other): val(std::move(other.val)) {}
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
      uint16_t len;
      for (len = 0; len < SIZE && str.get()[len] != 0; ++len) {}
      auto val = std::make_shared<InnerStorage>();
      val->fill(0);
      memcpy(val->data(), (uint8_t*) str.get(), len);
      return val;
    }

    static Result<Storage<SIZE>, enum ParseError> fromString(const StringView str) {
      if (strlen(str.get()) > SIZE) {
        return Result<Storage<SIZE>, ParseError>(ParseError::TOO_BIG);
      }
      auto val = std::make_shared<InnerStorage>();
      val->fill(0);
      memcpy(val.get(), str.get(), SIZE);
      return Result<Storage<SIZE>, enum ParseError>(val);
    }
  };

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
