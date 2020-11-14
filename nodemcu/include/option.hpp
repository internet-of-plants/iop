#ifndef IOP_OPTION_H_
#define IOP_OPTION_H_

#include <functional>
#include <panic.hpp>

/// Optional sum-type, may be filled and contain a T, or not be filled and unable to access it
/// methods panic instead of causing undefined behavior
///
/// Most methods move out by default.
template<typename T>
class Option {
 private:
  bool filled;
  union {
    uint8_t dummy;
    T value;
  };

  void reset() noexcept {
    if (this->isSome()) {
      if (!std::is_pointer<T>() && !std::is_reference<T>() && !std::is_trivially_destructible<T>()) {
        this->value.~T();
      }
    }
    this->dummy = 0;
    this->filled = false;
  }
 public:
  constexpr Option() noexcept: filled(false), dummy(0) {}
  constexpr Option(T v) noexcept: filled(true), value(std::move(v)) {}
  Option(Option<T>& other) = delete;
  void operator=(Option<T>& other) = delete;
  void operator=(Option<T>&& other) noexcept {
    this->filled = other.filled;
    if (other.filled) {
      this->value = std::move(other.value);
      other.dummy = 0;
    } else {
      this->dummy = 0;
    }
  }

  Option(Option<T>&& other) noexcept {
    this->filled = other.filled;
    if (other.filled) {
      other.filled = false;
      this->value = std::move(other.value);
      other.dummy = 0;
    } else {
      this->dummy = 0;
    }
  }

  Option<std::reference_wrapper<T>> asMut() {
    if (this->isSome()) {
      T& ref = this->value;
      const std::reference_wrapper<T> refWrapper(ref);
      return Option<std::reference_wrapper<T>>(refWrapper);
    }
    return Option<std::reference_wrapper<T>>();
  }

  Option<std::reference_wrapper<const T>> asRef() const {
    if (this->isSome()) {
      const T& ref = this->value;
      const std::reference_wrapper<const T> refWrapper(ref);
      return Option<std::reference_wrapper<const T>>(refWrapper);
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

  T expect(const StringView msg) {
    if (this->isNone()) { panic_(msg); }
    this->filled = false;
    T value = std::move(this->value);
    this->dummy = 0;
    return value;
  }
  T expect(const StaticString msg) {
    if (this->isNone()) { panic_(msg); }
    this->filled = false;
    T value = std::move(this->value);
    this->dummy = 0;
    return value;
  }
  T unwrap() noexcept { return this->expect(F("Tried to unwrap an empty Option")); }
  T unwrapOr(const T or_) noexcept {
    if (this->isSome()) { return this->expect(F("unwrap_or is broken")); }
    return or_;
  }

  template <typename U>
  Option<U> andThen(std::function<Option<U> (const T)> f) noexcept {
    if (this->isSome()) {
      return f(this->expect(F("Option::map, is none but shouldn't")));
    }
    return Option<U>();
  }

  template <typename U>
  Option<U> map(std::function<U (const T)> f) noexcept {
    if (this->isSome()) {
      return f(this->expect(F("Option::map, is none but shouldn't")));
    }
    return Option<U>();
  }

  Option<T> or_(Option<T> v) {
    if (this->isSome()) return std::move(*this);
    return v;
  }

  Option<T> orElse(std::function<Option<T>()> v) {
    if (this->isSome()) return std::move(this);
    return v();
  }
};

#endif