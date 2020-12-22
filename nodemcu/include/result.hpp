#ifndef IOP_RESULT_H_
#define IOP_RESULT_H_

#include <functional>
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

  bool isEmpty() noexcept;
  void reset() noexcept;
 public:
  Result(T v) noexcept: kind_(OK), ok(std::move(v)) {}
  Result(E e) noexcept: kind_(ERROR), error(std::move(e)) {}
  Result(Result<T, E>& other) = delete;
  Result<T, E>& operator=(Result<T, E>& other) = delete;
  Result<T, E>& operator=(Result<T, E>&& other) noexcept;
  Result(Result<T, E>&& other) noexcept;
  ~Result() noexcept { this->reset(); }
  bool isOk() const noexcept;
  bool isErr() const noexcept;
  T expectOk(const StringView msg) noexcept;
  T expectOk(const StaticString msg) noexcept;
  T unwrapOk() noexcept;
  T unwrapOkOr(const T or_) noexcept;
  E expectErr(const StringView msg) noexcept;
  E expectErr(const StaticString msg) noexcept;
  E unwrapErr() noexcept;
  E unwrapErrOr(const E or_) noexcept;

  template<typename U>
  Result<U, E> mapOk(std::function<U (const T)> f) noexcept {
    // TODO: This declaration should be at result.cpp, but we didn't manage to make it compile
    if (this->isEmpty()) panic_(F("Result is empty, at mapOk"));
    else if (this->isOk()) {
      const auto val = f(std::move(this->ok));
      this->reset();
      return val;
    } else /*if (this->isErr())*/ {
      const auto val = std::move(this->error);
      this->reset();
      return val;
    }
  }
};

#endif