#ifndef IOP_RESULT_H_
#define IOP_RESULT_H_

#include <panic.hpp>

/// Result sumtype, may be ok and contain a T, or not be ok and contain an E
/// This type can be moved out, so it _will_ be empty, it will panic if tried to access after
/// methods panic instead of causing undefined behavior
///
/// Most methods move out by default.
template<typename T, typename E>
class Result {
 private:
  enum ResultKind {
    OK,
    ERROR,
    EMPTY
  };
  enum ResultKind kind_;
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
    this->kind_ = EMPTY;
  }
 public:
  Result(T v) noexcept: kind_(OK), ok(std::move(v)) {}
  Result(E e) noexcept: kind_(ERROR), error(std::move(e)) {}
  Result(Result<T, E>& other) = delete;
  void operator=(Result<T, E>& other) = delete;
  void operator=(Result<T, E>&& other) noexcept {
    this->kind_ = other.kind_;
    other.kind_ = EMPTY;
    switch (this->kind_) {
      case OK:
        this->ok = std::move(other.ok);
        break;
      case ERROR:
        this->error = std::move(other.error);
        break;
      case EMPTY:
        panic_(F("Tried to move out of an empty result, at operator="));
        break;
    }
    other.dummy = 0;
  }

  Result(Result<T, E>&& other) noexcept {
    this->kind_ = other.kind_;
    other.kind_ = EMPTY;
    switch (this->kind_) {
      case OK:
        this->ok = std::move(other.ok);
        break;
      case ERROR:
        this->error = std::move(error);
        break;
      case EMPTY:
      panic_(F("Tried to move out of an empty result, at constructor"));
        break;
    }
    other.dummy = 0;
  }

  ~Result() noexcept { this->reset(); }

  bool isOk() const {
    switch (this->kind_) {
      case OK:
        return true;
      case ERROR:
        return false;
      case EMPTY:
        panic_(F("Result is empty, at isOk"));
    }
  }

  bool isErr() const {
    switch (this->kind_) {
      case ERROR:
        return true;
      case OK:
        return false;
      case EMPTY:
        panic_(F("Result is empty, at isErr"));
    }
  }

  T expectOk(const StringView msg) {
    if (this->isErr()) { panic_(msg); }
    this->kind_ = EMPTY;
    T value = std::move(this->ok);
    this->dummy = 0;
    return value;
  }

  T expectOk(const StaticString msg) {
    if (this->isErr()) { panic_(msg); }
    this->kind_ = EMPTY;
    T value = std::move(this->ok);
    this->dummy = 0;
    return value;
  }
  T unwrapOk() { return this->expectOk(F("Tried to unwrapOk an errored Result")); }
  T unwrapOkOr(const T or_) noexcept {
    if (this->isOk()) {
      return std::move(this->ok);
    } else if (this->isErr()) {
      return or_;
    } else {
      panic_("Result is empty, at unwrapOkOr");
    }
  }

  E expectErr(const StringView msg) {
    if (this->isOk()) { panic_(msg); }
    this->kind_ = EMPTY;
    E value = std::move(this->error);
    this->dummy = 0;
    return value;
  }
  E expectErr(const StaticString msg) {
    if (this->isOk()) { panic_(msg); }
    this->kind_ = EMPTY;
    E value = std::move(this->error);
    this->dummy = 0;
    return value;
  }
  E unwrapErr() noexcept { return this->expectErr(F("Tried to unwrapErr a Result that succeeded")); }
  E unwrapErrOr(const E or_) noexcept {
    if (this->isErr()) {
      return std::move(this->err);
    } else if (this->isOk()) {
      return or_;
    } else {
      panic_("Result is empty, at unwrapErrOr");
    }  }

  template <typename U>
  Result<U, E> mapOk(std::function<U (const T)> f) noexcept {
    if (this->isOk()) {
      this->kind_ = EMPTY;
      return Result<U, E>(f(std::move(this->ok)));
    } else {
      this->kind_ = EMPTY;
      return Result<U, E>(std::move(this->error));
    }
  }
};

#endif