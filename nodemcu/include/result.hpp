#ifndef IOP_RESULT_H_
#define IOP_RESULT_H_

#include <panic.hpp>

/// Result sumtype, may be ok and contain a T, or not be ok and contain an E
/// This type can be moved out, so it _will_ be empty, it will panic if tried to access after
/// a move instead of causing undefined behavior
///
/// Most methods move out by default.
///
/// Pwease no exception at T's or E's destructor
template<typename T, typename E>
class Result {
 private:
  enum ResultKind {
    OK = 40,
    ERROR,
    EMPTY
  };
  enum ResultKind kind_;
  union {
    uint8_t dummy;
    T ok;
    E error;
  };

  bool isEmpty() noexcept {
    return this->kind_ == EMPTY;
  }

  void reset() noexcept {
    switch (this->kind_) {
      case EMPTY:
        return;
      case OK:
        this->ok.~T();
        break;
      case ERROR:
        this->error.~E();
        break;
    }
    this->kind_ = EMPTY;
    this->dummy = 0;
  }
 public:
  Result(T v) noexcept: kind_(OK), ok(std::move(v)) {}
  Result(E e) noexcept: kind_(ERROR), error(std::move(e)) {}
  Result(Result<T, E>& other) = delete;
  Result<T, E>& operator=(Result<T, E>& other) = delete;
  Result<T, E>& operator=(Result<T, E>&& other) noexcept {
    this->kind_ = other.kind_;
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
    other.reset();
    return *this;
  }

  Result(Result<T, E>&& other) noexcept {
    this->kind_ = other.kind_;
    switch (this->kind_) {
      case OK:
        this->ok = std::move(other.ok);
        break;
      case ERROR:
        this->error = std::move(other.error);
        break;
      case EMPTY:
      panic_(F("Tried to move out of an empty result, at constructor"));
        break;
    }
    other.reset();
  }

  ~Result() noexcept { this->reset(); }

  bool isOk() const noexcept {
    switch (this->kind_) {
      case OK:
        return true;
      case ERROR:
        return false;
      case EMPTY:
      default:
        panic_(F("Result is empty, at isOk"));
    }
  }

  bool isErr() const noexcept {
    switch (this->kind_) {
      case OK:
        return true;
      case ERROR:
        return false;
      case EMPTY:
      default:
        panic_(F("Result is empty, at isErr"));
    }
  }

  T expectOk(const StringView msg) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at expectOk");
    if (this->isErr()) panic_(msg);
    const T value = std::move(this->ok);
    this->reset();
    return value;
  }

  T expectOk(const StaticString msg) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at expectOk");
    if (this->isErr()) panic_(msg);
    const T value = std::move(this->ok);
    this->reset();
    return value;
  }
  T unwrapOk() noexcept {
    if (this->isEmpty()) panic_("Result is empty, at unwrapOk");
    return this->expectOk(F("Tried to unwrapOk an errored Result"));
 }
  T unwrapOkOr(const T or_) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at unwrapOkOr");
    if (this->isOk()) {
      const auto val = std::move(this->ok);
      this->reset();
      return val;
    } else if (this->isErr()) {
      return or_;
    }
  }

  E expectErr(const StringView msg) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at expectErr");
    if (this->isOk()) panic_(msg);
    const E value = std::move(this->error);
    this->reset();
    return value;
  }
  E expectErr(const StaticString msg) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at expectErr");
    if (this->isOk()) panic_(msg);
    const E value = std::move(this->error);
    this->reset();
    return value;
  }
  E unwrapErr() noexcept {
    if (this->isEmpty()) panic_("Result is empty, at unwrapErr");
    return this->expectErr(F("Tried to unwrapErr a Result that succeeded"));
  }
  E unwrapErrOr(const E or_) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at unwrapErrOr");
    if (this->isErr()) {
      const auto val = std::move(this->err);
      this->reset();
      return val;
    } else if (this->isOk()) {
      return or_;
    }
  }

  template <typename U>
  Result<U, E> mapOk(std::function<U (const T)> f) noexcept {
    if (this->isEmpty()) panic_("Result is empty, at mapOk");
    if (this->isOk()) {
      const auto val = f(std::move(this->ok));
      this->reset();
      return val;
    } else if (this->isErr()) {
      const auto val = std::move(this->error);
      this->reset();
      return val;
    }
  }
};

#endif