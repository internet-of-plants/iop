#ifndef IOP_OPTION_H_
#define IOP_OPTION_H_

#include <fixed_string.hpp>
#include <functional>
#include <panic.hpp>
#include <string_view.hpp>
#include <utils.hpp>

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
    if (this->isSome()) {
      // TODO: do we really need this checks? If so we also need them at Result.
      // They were taken from the std::optional spec
      // if (!std::is_pointer<T>() && !std::is_reference<T>() &&
      // !std::is_trivially_destructible<T>()) {
      this->value.~T();
      //}
    }
    this->dummy = 0;
    this->filled = false;
  }

public:
  constexpr Option() noexcept : filled(false), dummy(0) {}
  constexpr Option(T v) noexcept : filled(true), value(std::move(v)) {}
  Option(Option<T> &other) = delete;
  Option<T> &operator=(Option<T> &other) = delete;
  Option<T> &operator=(Option<T> &&other) noexcept {
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
    this->reset();
    this->filled = other.filled;
    if (other.filled) {
      this->value = std::move(other.value);
    } else {
      this->dummy = 0;
    }
    other.reset();
  }

  Option<std::reference_wrapper<T>> asMut() noexcept {
    if (this->isSome()) {
      return std::reference_wrapper<T>(this->value);
    }
    return Option<std::reference_wrapper<T>>();
  }

  Option<std::reference_wrapper<const T>> asRef() const noexcept {
    if (this->isSome()) {
      return std::reference_wrapper<const T>(this->value);
    }
    return Option<std::reference_wrapper<const T>>();
  }

  ~Option() noexcept { this->reset(); }

  constexpr bool isSome() const noexcept { return filled; }
  constexpr bool isNone() const noexcept { return !this->isSome(); }
  Option<T> take() noexcept {
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

  T expect(const StringView msg) noexcept {
    if (this->isNone()) {
      panic_(msg);
    }
    const T value = std::move(this->value);
    this->reset();
    return value;
  }
  T expect(const StaticString msg) noexcept {
    if (this->isNone()) {
      panic_(msg);
    }
    T value = std::move(this->value);
    this->reset();
    return value;
  }
  T unwrap() noexcept {
    return this->expect(F("Tried to unwrap an empty Option"));
  }
  T unwrapOr(T or_) noexcept {
    if (this->isSome()) {
      return this->expect(F("unwrapOr is broken"));
    }
    return or_;
  }

  template <typename U>
  Option<U> andThen(std::function<Option<U>(const T)> f) noexcept {
    if (this->isSome())
      return f(this->expect(F("Option::map, is none but shouldn't")));
    return Option<U>();
  }

  template <typename U> Option<U> map(std::function<U(const T)> f) noexcept {
    if (this->isSome())
      return f(this->expect(F("Option::map, is none but shouldn't")));
    return Option<U>();
  }

  Option<T> or_(Option<T> v) noexcept {
    if (this->isSome())
      return std::move(this);
    return v;
  }

  Option<T> orElse(std::function<Option<T>()> v) noexcept {
    if (this->isSome())
      return std::move(this);
    return v();
  }
};

#endif