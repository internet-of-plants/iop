#ifndef IOP_RESULT_HPP
#define IOP_RESULT_HPP

#include "option.hpp"
#include "string_view.hpp"
#include <functional>

#include "panic.hpp"
#include "static_string.hpp"

#define RESULT_MAP_OK(res, type, func)                                         \
  res.mapOk<type>(func, F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)

#define UNWRAP_OK_OR(res, or_)                                                 \
  std::move((res).unwrapOkOr(or_, F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC))
#define UNWRAP_ERR_OR(res, or_)                                                \
  std::move((res).unwrapErrOr(or_, F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC))

#define UNWRAP_OK(res)                                                         \
  std::move(                                                                   \
      (res).expectOk(F(#res " isn't Ok"), CUTE_FILE, CUTE_LINE, CUTE_FUNC))
#define UNWRAP_OK_REF(res) UNWRAP_OK(RESULT_AS_REF(res)).get()
#define UNWRAP_OK_MUT(res) UNWRAP_OK(RESULT_AS_MUT(res)).get()

#define UNWRAP_ERR(r)                                                          \
  std::move((r).expectErr(F(#r " isn't Err"), CUTE_FILE, CUTE_LINE, CUTE_FUNC))
#define UNWRAP_ERR_REF(res) UNWRAP_ERR(RESULT_AS_REF(res)).get()
#define UNWRAP_ERR_MUT(res) UNWRAP_ERR(RESULT_AS_MUT(res)).get()

#define RESULT_AS_REF(res) res.asRef(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)
#define RESULT_AS_MUT(res) res.asMut(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)

#define IS_OK(res) res.isOk(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)
#define IS_ERR(res) res.isErr(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)

#define RESULT_OK(res) res.ok(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)
#define RESULT_ERR(res) res.err(F(#res), CUTE_FILE, CUTE_LINE, CUTE_FUNC)

/// Result sumtype, may be ok and contain a T, or not be ok and contain an E
/// This type can be moved out, so it _will_ be empty, it will panic if tried to
/// access after a move instead of causing undefined behavior
///
/// You should probably use the macros defined above, instead of directly using
/// Result's methods. They will report where empty results were accessed.
/// Most used macros will be IS_{OK, ERR}, UNWRAP_{OK, ERR}_{REF, MUT}
///
/// Most methods move out by default. You probably want to call `.asRef()`
/// before moving it out (UNWRAP_REF)
///
/// Pwease no exception at T's or E's destructor
template <typename T, typename E> class Result {
private:
  enum ResultKind { OK = 40, ERROR, EMPTY };
  enum ResultKind kind_;
  union {
    uint8_t dummy;
    T success;
    E error;
  };

  auto isEmpty() const noexcept -> bool { return this->kind_ == EMPTY; }
  void reset() noexcept {
    switch (this->kind_) {
    case EMPTY:
      return;
    case OK:
      this->success.~T();
      break;
    case ERROR:
      this->error.~E();
      break;
    }
  }

public:
  // NOLINTNEXTLINE hicpp-explicit-conversions
  Result(T v) noexcept : kind_(OK), success(std::move(v)) {}
  // NOLINTNEXTLINE hicpp-explicit-conversions
  Result(E e) noexcept : kind_(ERROR), error(std::move(e)) {}
  Result(Result<T, E> &other) = delete;
  auto operator=(Result<T, E> &other) -> Result<T, E> & = delete;
  auto operator=(Result<T, E> &&other) noexcept -> Result<T, E> & {
    this->reset();
    this->kind_ = other.kind_;
    switch (this->kind_) {
    case OK:
      this->success = std::move(other.success);
      break;
    case ERROR:
      this->error = std::move(other.error);
      break;
    case EMPTY:
      panic_(F("Result is empty"));
      break;
    }
    other.reset();
    return *this;
  }
  Result(Result<T, E> &&other) noexcept {
    this->reset();
    this->kind_ = other.kind_;
    switch (this->kind_) {
    case OK:
      this->success = std::move(other.success);
      break;
    case ERROR:
      this->error = std::move(other.error);
      break;
    case EMPTY:
      panic_(F("Result is empty"));
      break;
    }
    other.reset();
  }
  ~Result() noexcept { this->reset(); }
  auto isOk() const noexcept -> bool {
    switch (this->kind_) {
    case OK:
      return true;
    case ERROR:
      return false;
    case EMPTY:
    default:
      panic_(F("Result is empty"));
    }
  }
  auto isErr() const noexcept -> bool {
    switch (this->kind_) {
    case OK:
      return true;
    case ERROR:
      return false;
    case EMPTY:
    default:
      panic_(F("Result is empty"));
    }
  }
  auto expectOk(const StringView msg) noexcept -> T {
    if (IS_ERR(*this))
      panic_(msg);

    T value = std::move(this->success);
    this->reset();
    return value;
  }
  auto expectOk(const StaticString msg) noexcept -> T {
    if (IS_ERR(*this))
      panic_(msg);

    T value = std::move(this->success);
    this->reset();
    return value;
  }
  auto unwrapOkOr(const T or_) noexcept -> T {
    if (IS_ERR(*this))
      return or_;

    const auto val = std::move(this->success);
    this->reset();
    return val;
  }
  auto expectErr(const StringView msg) noexcept -> E {
    if (IS_OK(*this))
      panic_(msg);

    const E value = std::move(this->error);
    this->reset();
    return value;
  }
  auto expectErr(const StaticString msg) noexcept -> E {
    if (IS_OK(*this))
      panic_(msg);

    E value = std::move(this->error);
    this->reset();
    return value;
  }
  auto unwrapErrOr(const E or_) noexcept -> E {
    if (IS_OK(*this))
      return or_;

    const auto val = std::move(this->err);
    this->reset();
    return val;
  }

  auto asMut() noexcept
      -> Result<std::reference_wrapper<T>, std::reference_wrapper<E>> {
    if (IS_OK(*this)) {
      return std::reference_wrapper<T>(this->success);
    } else {
      return std::reference_wrapper<E>(this->error);
    }
  }

  auto asRef() const noexcept -> Result<std::reference_wrapper<const T>,
                                        std::reference_wrapper<const E>> {
    if (IS_OK(*this)) {
      return std::reference_wrapper<const T>(this->success);
    } else {
      return std::reference_wrapper<const E>(this->error);
    }
  }

  template <typename U>
  auto mapOk(std::function<U(const T)> f) noexcept -> Result<U, E> {
    if (IS_OK(*this)) {
      auto val = f(std::move(this->success));
      this->reset();
      return std::move(val);
    } else {
      auto val = std::move(this->error);
      this->reset();
      return std::move(val);
    }
  }

  auto ok() noexcept -> Option<T> {
    if (IS_ERR(*this))
      return Option<T>();

    auto val = std::move(this->success);
    this->reset();
    return std::move(val);
  }

  auto err() noexcept -> Option<T> {
    if (IS_OK(*this))
      return Option<T>();

    auto val = std::move(this->error);
    this->reset();
    return std::move(val);
  }

  // Allows more detailed panics, used by UNWRAP(_OK, _ERR)(_REF, _MUT) macros
  auto asMut(const StaticString varName, const StaticString file,
             const uint32_t line, const StringView func) noexcept
      -> Result<std::reference_wrapper<T>, std::reference_wrapper<E>> {
    if (this->isOk(varName, file, line, func)) {
      return std::reference_wrapper<T>(this->success);
    } else {
      return std::reference_wrapper<E>(this->error);
    }
  }

  auto asRef(const StaticString varName, const StaticString file,
             const uint32_t line, const StringView func) const noexcept
      -> Result<std::reference_wrapper<const T>,
                std::reference_wrapper<const E>> {
    if (this->isOk(varName, file, line, func)) {
      return std::reference_wrapper<const T>(this->success);
    } else {
      return std::reference_wrapper<const E>(this->error);
    }
  }

  auto expectOk(const StringView msg, const StaticString file,
                const uint32_t line, const StringView func) noexcept -> T {
    if (this->isErr(msg, file, line, func))
      panic__(msg, file, line, func);

    T value = std::move(this->success);
    this->reset();
    return value;
  }
  auto expectOk(const StaticString msg, const StaticString file,
                const uint32_t line, const StringView func) noexcept -> T {
    if (this->isErr(msg, file, line, func))
      panic__(msg, file, line, func);

    T value = std::move(this->success);
    this->reset();
    return value;
  }
  auto expectErr(const StringView msg, const StaticString file,
                 const uint32_t line, const StringView func) noexcept -> E {
    if (this->isOk(msg, file, line, func))
      panic__(msg, file, line, func);

    T value = std::move(this->error);
    this->reset();
    return value;
  }
  auto expectErr(const StaticString msg, const StaticString file,
                 const uint32_t line, const StringView func) noexcept -> E {
    if (this->isOk(msg, file, line, func))
      panic__(msg, file, line, func);

    E value = std::move(this->error);
    this->reset();
    return value;
  }
  auto ok(const StaticString varName, const StaticString file,
          const uint32_t line, const StringView func) noexcept -> Option<T> {
    if (this->isErr(varName, file, line, func))
      return Option<T>();

    const auto val = std::move(this->success);
    this->reset();
    return val;
  }

  auto err(const StaticString varName, const StaticString file,
           const uint32_t line, const StringView func) noexcept -> Option<T> {
    if (this->isOk(varName, file, line, func))
      return Option<T>();

    T value = std::move(this->error);
    this->reset();
    return value;
  }

  template <typename U>
  auto mapOk(std::function<U(const T)> f, const StaticString varName,
             const StaticString file, const uint32_t line,
             const StringView func) noexcept -> Result<U, E> {
    if (this->isOk(varName, file, line, func)) {
      const auto val = f(std::move(this->success));
      this->reset();
      return val;

    } else {
      const auto val = std::move(this->error);
      this->reset();
      return val;
    }
  }
  auto unwrapOkOr(const T or_, const StaticString varName,
                  const StaticString file, const uint32_t line,
                  const StringView func) noexcept -> T {
    if (this->isErr(varName, file, line, func))
      return or_;

    const auto val = std::move(this->success);
    this->reset();
    return val;
  }
  auto unwrapErrOr(const E or_, const StaticString varName,
                   const StaticString file, const uint32_t line,
                   const StringView func) noexcept -> E {
    if (this->isOk(varName, file, line, func))
      return or_;

    const auto val = std::move(this->error);
    this->reset();
    return val;
  }
  auto isOk(const StaticString varName, const StaticString file,
            const uint32_t line, const StringView func) const noexcept -> bool {
    switch (this->kind_) {
    case OK:
      return true;
    case ERROR:
      return false;
    case EMPTY:
    default:
      panic__(String(F("Empty Result: ")) + varName.get(), file, line, func);
    }
  }
  auto isErr(const StaticString varName, const StaticString file,
             const uint32_t line, const StringView func) const noexcept
      -> bool {
    return !this->isOk(varName, file, line, func);
  }
};

#endif