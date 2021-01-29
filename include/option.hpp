#ifndef IOP_OPTION_HPP
#define IOP_OPTION_HPP

#include "certificate_storage.hpp"

#include "string_view.hpp"

#include "panic.hpp"

#include "static_string.hpp"
#include "string_view.hpp"
#include "tracer.hpp"

#include <functional>

#define UNWRAP(opt)                                                            \
  std::move((opt).expect(F(#opt " is None"), CUTE_FILE, CUTE_LINE, CUTE_FUNC))
#define UNWRAP_REF(opt) UNWRAP((opt).asRef()).get()
#define UNWRAP_MUT(opt) UNWRAP((opt).asMut()).get()

/// Optional sum-type, may be filled and contain a T, or not be filled and
/// unable to access it methods panic instead of causing undefined behavior
///
/// Most methods move out by default. You probably want to call `.asRef()`
/// before moving it out.
///
/// Pwease no exception at T's destructor
template <typename T> class Option {
private:
  bool filled;
  union {
    uint8_t dummy;
    T value;
  };

  void reset() noexcept {
    IOP_TRACE();
    if (this->filled) {
      this->value.~T();
      this->dummy = 0;
      this->filled = false;
    }
  }

public:
  Option() noexcept : filled(false), dummy(0) { IOP_TRACE(); }
  // NOLINTNEXTLINE hicpp-explicit-conversions *-member-init
  Option(T v) noexcept : filled(true), value(std::move(v)) { IOP_TRACE(); }
  Option(Option<T> const &other) = delete;
  auto operator=(Option<T> const &other) -> Option<T> & = delete;
  auto operator=(Option<T> &&other) noexcept -> Option<T> & {
    IOP_TRACE();

    if (this == &other)
      return *this;

    this->reset();
    this->filled = other.filled;
    if (other.filled) {
      this->value = std::move(other.value);
    } else {
      this->dummy = 0;
    }
    other.reset();
    return *this;
  }

  Option(Option<T> &&other) noexcept {
    IOP_TRACE();
    this->reset();
    this->filled = other.filled;
    if (other.filled) {
      this->value = std::move(other.value);
    } else {
      this->dummy = 0;
    }
    other.reset();
  }

  auto asMut() noexcept -> Option<std::reference_wrapper<T>> {
    IOP_TRACE();
    if (this->isSome()) {
      return std::ref(this->value);
    }
    return Option<std::reference_wrapper<T>>();
  }

  auto asRef() const noexcept -> Option<std::reference_wrapper<const T>> {
    IOP_TRACE();
    if (this->isSome()) {
      return std::ref<const T>(this->value);
    }
    return Option<std::reference_wrapper<const T>>();
  }

  ~Option() noexcept {
    IOP_TRACE();
    this->reset();
  }

  auto isSome() const noexcept -> bool {
    IOP_TRACE();
    return filled;
  }
  auto isNone() const noexcept -> bool {
    IOP_TRACE();
    return !filled;
  }
  auto take() noexcept -> Option<T> {
    IOP_TRACE();
    Option<T> other;
    other.filled = this->filled;
    if (this->filled) {
      other.value = std::move(this->value);
      this->reset();
    } else {
      other.dummy = 0;
    }
    return other;
  }

  auto expect(const StringView msg) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      panic_(msg);

    const T value = std::move(this->value);
    this->reset();
    return value;
  }
  auto expect(const StaticString msg) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      panic_(msg);

    T value = std::move(this->value);
    this->reset();
    return value;
  }
  auto unwrapOr(T or_) noexcept -> T {
    IOP_TRACE();
    if (this->isSome())
      return this->expect(F("unwrapOr is broken"));
    return or_;
  }

  template <typename U>
  auto andThen(std::function<Option<U>(const T)> f) noexcept -> Option<U> {
    IOP_TRACE();
    if (this->isSome())
      return f(this->expect(F("Option::map, is none but shouldn't")));
    return Option<U>();
  }

  template <typename U>
  auto map(std::function<U(const T)> f) noexcept -> Option<U> {
    IOP_TRACE();
    if (this->isSome())
      return f(this->expect(F("Option::map, is none but shouldn't")));
    return Option<U>();
  }

  auto or_(Option<T> v) noexcept -> Option<T> {
    IOP_TRACE();
    if (this->isSome())
      return std::move(this);
    return v;
  }

  auto orElse(std::function<Option<T>()> v) noexcept -> Option<T> {
    IOP_TRACE();
    if (this->isSome())
      return std::move(this);
    return v();
  }

  // Allows for more detailed panics, used by UNWRAP(_REF, _MUT) macros
  auto expect(const StringView msg, const StaticString file,
              const uint32_t line, const StringView func) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      panic__(msg, file, line, func);

    const T value = std::move(this->value);
    this->reset();
    return value;
  }
  auto expect(const StaticString msg, const StaticString file,
              const uint32_t line, const StringView func) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      panic__(msg, file, line, func);

    T value = std::move(this->value);
    this->reset();
    return value;
  }
};

#endif