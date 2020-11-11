#ifndef IOP_RESULT_H_
#define IOP_RESULT_H_

#include <panic.hpp>

template<typename T, typename E>
class Result {
 private:
  enum ResultKind {
    OK,
    ERROR,
    EMPTY
  };
  enum ResultKind kind;
  union {
    uint8_t dummy;
    T ok;
    E error;
  };

  void reset() noexcept {
    if (this->isOk()) {
      this->ok.~T();
    } else if (this->isErr()) {
      this->error.~E();
    }
    this->dummy = 0;
    this->kind = EMPTY;
  }
 public:
  Result(T v) noexcept: kind(OK), ok(std::move(v)) {}
  Result(E e) noexcept: kind(ERROR), error(std::move(e)) {}
  Result(Result<T, E>& other) = delete;
  void operator=(Result<T, E>& other) = delete;
  void operator=(Result<T, E>&& other) noexcept {
    this->kind = other.kind;
    other.kind = EMPTY;
    switch (this->kind) {
      case OK:
        this->ok = std::move(other.ok);
        break;
      case ERROR:
        this->error = std::move(other.error);
        break;
      case EMPTY:
        break;
    }
    other.dummy = 0;
  }

  Result(Result<T, E>&& other) noexcept {
    this->kind = other.kind;
    other.kind = EMPTY;
    switch (this->kind) {
      case OK:
        this->ok = std::move(other.ok);
        break;
      case ERROR:
        this->error = std::move(error);
        break;
      case EMPTY:
        break;
    }
    other.dummy = 0;
  }


  ~Result() noexcept { this->reset(); }

  constexpr bool isOk() const noexcept {
    switch (this->kind) {
      case OK:
        return true;
      case ERROR:
      case EMPTY:
      default:
        return false;
    }
  }

  constexpr bool isErr() const noexcept {
    switch (this->kind) {
      case ERROR:
        return true;
      case OK:
      case EMPTY:
      default:
        return false;
    }
  }

  T expectOk(const StringView msg) noexcept {
    if (this->isErr()) { panic_(msg); }
    this->kind = EMPTY;
    T value = std::move(this->ok);
    this->dummy = 0;
    return std::move(value);
  }

  T expectOk(const StaticString msg) noexcept {
    if (this->isErr()) { panic_(msg); }
    this->kind = EMPTY;
    T value = std::move(this->ok);
    this->dummy = 0;
    return std::move(value);
  }
  T unwrapOk() noexcept { return this->expectOk(F("Tried to unwrapOk an errored Result")); }
  T unwrapOk_or(const T or_) noexcept {
    if (this->isSome()) { return std::move(this->expectOk(F("unwrapOk_or is broken"))); }
    return std::move(or_);
  }

  E expectErr(const StringView msg) noexcept {
    if (this->isOk()) { panic_(msg); }
    this->kind = EMPTY;
    E value = std::move(this->error);
    this->dummy = 0;
    return std::move(value);
  }
  E expectErr(const StaticString msg) noexcept {
    if (this->isOk()) { panic_(msg); }
    this->kind = EMPTY;
    E value = std::move(this->error);
    this->dummy = 0;
    return std::move(value);
  }
  E unwrapErr() noexcept { return this->expectErr(F("Tried to unwrapErr a Result that succeeded")); }
  E unwrapErr_or(const E or_) noexcept {
    if (this->isErr()) { return std::move(this->expectErr(F("unwrapErr_or is broken"))); }
    return std::move(or_);
  }
};

#endif
