#ifndef IOP_CORE_RESULT_HPP
#define IOP_CORE_RESULT_HPP

#include "core/option.hpp"

#define RESULT_MAP_OK(res, type, func)                                         \
  res.mapOk<type>(func, F(#res), IOP_CODE_POINT())

#define UNWRAP_OK_OR(res, or_)                                                 \
  std::move((res).unwrapOkOr(or_, F(#res), IOP_CODE_POINT())
#define UNWRAP_ERR_OR(res, or_)                                                \
  std::move((res).unwrapErrOr(or_, F(#res), IOP_CODE_POINT()))

#define UNWRAP_OK(res)                                                         \
  std::move((res).expectOk(F(#res " isn't Ok"), IOP_CODE_POINT()))
#define UNWRAP_OK_REF(res) UNWRAP_OK(RESULT_AS_REF(res)).get()
#define UNWRAP_OK_MUT(res) UNWRAP_OK(RESULT_AS_MUT(res)).get()

#define UNWRAP_ERR(r)                                                          \
  std::move((r).expectErr(F(#r " isn't Err"), IOP_CODE_POINT()))
#define UNWRAP_ERR_REF(res) UNWRAP_ERR(RESULT_AS_REF(res)).get()
#define UNWRAP_ERR_MUT(res) UNWRAP_ERR(RESULT_AS_MUT(res)).get()

#define RESULT_AS_REF(res) res.asRef(F(#res), IOP_CODE_POINT())
#define RESULT_AS_MUT(res) res.asMut(F(#res), IOP_CODE_POINT())

#define IS_OK(res) res.isOk(F(#res), IOP_CODE_POINT())
#define IS_ERR(res) res.isErr(F(#res), IOP_CODE_POINT())

#define RESULT_OK(res) res.ok(F(#res), IOP_CODE_POINT())
#define RESULT_ERR(res) res.err(F(#res), IOP_CODE_POINT())

namespace iop {

/// Result sumtype, may be ok and contain a T, or not be ok and contain an E
/// This type can be moved out, so it _will_ be empty, it will panicHandler if
/// tried to access after a move (you can only destroy it).
///
/// You should probably use the macros defined above, instead of directly using
/// Result's methods. They will ergonomically report where empty results were
/// accessed. Most used macros will be IS_{OK, ERR}, UNWRAP_{OK, ERR}_{REF, MUT}
///
/// Most methods move out by default. You probably want to call `.asRef()`
/// before moving it out (or use UNWRAP_{OK, ERR}_{REF, MUT})
///
/// Exceptions in T's and E's destructors will trigger abort
template <typename T, typename E> class Result {
private:
  enum class ResultKind { EMPTY = 0, OK, ERROR };
  ResultKind kind_;
  union {
    uint8_t dummy;
    T success;
    E error;
  };

  auto isEmpty() const noexcept -> bool {
    IOP_TRACE();
    return this->kind_ == ResultKind::EMPTY;
  }
  void reset() noexcept {
    IOP_TRACE();
    switch (this->kind_) {
    case ResultKind::EMPTY:
      return;
    case ResultKind::OK:
      this->success.~T();
      break;
    case ResultKind::ERROR:
      this->error.~E();
      break;
    }
    this->dummy = 0;
    this->kind_ = ResultKind::EMPTY;
  }

public:
  // NOLINTNEXTLINE hicpp-explicit-conversions
  Result(T v) noexcept : kind_(ResultKind::OK), success(std::move(v)) {
    IOP_TRACE();
  }
  // NOLINTNEXTLINE hicpp-explicit-conversions
  Result(E e) noexcept : kind_(ResultKind::ERROR), error(std::move(e)) {
    IOP_TRACE();
  }
  Result(Result<T, E> &other) = delete;
  auto operator=(Result<T, E> &other) -> Result<T, E> & = delete;
  auto operator=(Result<T, E> &&other) noexcept -> Result<T, E> & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->reset();
    this->kind_ = other.kind_;
    switch (this->kind_) {
    case ResultKind::OK:
      this->success = std::move(other.success);
      break;
    case ResultKind::ERROR:
      this->error = std::move(other.error);
      break;
    case ResultKind::EMPTY:
      iop_panic(F("Result is empty"));
      break;
    }
    other.reset();
    return *this;
  }
  Result(Result<T, E> &&other) noexcept {
    IOP_TRACE();
    this->reset();
    this->kind_ = other.kind_;
    switch (this->kind_) {
    case ResultKind::OK:
      this->success = std::move(other.success);
      break;
    case ResultKind::ERROR:
      this->error = std::move(other.error);
      break;
    case ResultKind::EMPTY:
      iop_panic(F("Result is empty"));
      break;
    }
    other.reset();
  }
  ~Result() noexcept {
    IOP_TRACE();
    this->reset();
  }

  // Use the macros, not this methods directly
  // Allows more detailed panics, used by UNWRAP(_OK,_ERR)(_REF,_MUT) macros
  auto asMut(const StaticString varName, const CodePoint point) noexcept
      -> Result<std::reference_wrapper<T>, std::reference_wrapper<E>> {
    IOP_TRACE();
    if (this->isOk(varName, point)) {
      return std::reference_wrapper<T>(this->success);
    } else {
      return std::reference_wrapper<E>(this->error);
    }
  }

  auto asRef(const StaticString varName, const CodePoint point) const noexcept
      -> Result<std::reference_wrapper<const T>,
                std::reference_wrapper<const E>> {
    IOP_TRACE();
    if (this->isOk(varName, point)) {
      return std::reference_wrapper<const T>(this->success);
    } else {
      return std::reference_wrapper<const E>(this->error);
    }
  }
  auto expectOk(const StaticString msg, const CodePoint point) noexcept -> T {
    IOP_TRACE();
    if (this->isErr(msg, point))
      iop::panicHandler(msg, point);
    T value = std::move(this->success);
    this->reset();
    return value;
  }
  auto expectErr(const StaticString msg, const CodePoint point) noexcept -> E {
    IOP_TRACE();
    if (this->isOk(msg, point))
      iop::panicHandler(msg, point);

    E value = std::move(this->error);
    this->reset();
    return value;
  }
  auto ok(const StaticString varName, const CodePoint point) noexcept
      -> Option<T> {
    IOP_TRACE();
    if (this->isErr(varName, point))
      return Option<T>();

    const auto val = std::move(this->success);
    this->reset();
    return val;
  }

  auto err(const StaticString varName, const CodePoint point) noexcept
      -> Option<T> {
    IOP_TRACE();
    if (this->isOk(varName, point))
      return Option<T>();

    T value = std::move(this->error);
    this->reset();
    return value;
  }

  template <typename U>
  auto mapOk(std::function<U(const T)> f, const StaticString varName,
             const CodePoint point) noexcept -> Result<U, E> {
    IOP_TRACE();
    if (this->isOk(varName, point)) {
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
                  const CodePoint point) noexcept -> T {
    IOP_TRACE();
    if (this->isErr(varName, point))
      return or_;

    const auto val = std::move(this->success);
    this->reset();
    return val;
  }
  auto unwrapErrOr(const E or_, const StaticString varName,
                   const CodePoint point) noexcept -> E {
    IOP_TRACE();
    if (this->isOk(varName, point))
      return or_;

    const auto val = std::move(this->error);
    this->reset();
    return val;
  }
  auto isOk(const StaticString varName, const CodePoint point) const noexcept
      -> bool {
    IOP_TRACE();
    switch (this->kind_) {
    case ResultKind::OK:
      return true;
    case ResultKind::ERROR:
      return false;
    case ResultKind::EMPTY:
    default:
      iop::panicHandler(String(F("Empty Result: ")) + varName.get(), point);
    }
  }
  auto isErr(const StaticString varName, const CodePoint point) const noexcept
      -> bool {
    IOP_TRACE();
    switch (this->kind_) {
    case ResultKind::OK:
      return false;
    case ResultKind::ERROR:
      return true;
    case ResultKind::EMPTY:
    default:
      iop::panicHandler(String(F("Empty Result: ")) + varName.get(), point);
    }
  }
};

} // namespace iop

#endif