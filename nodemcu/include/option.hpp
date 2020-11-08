#ifndef IOP_OPTION_H_
#define IOP_OPTION_H_

#include <functional>
#include <panic.hpp>

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
  Option(const T v) noexcept: filled(true), value(std::move(v)) {}
  Option(Option<T>& other) = delete;
  void operator=(Option<T>& other) = delete;
  void operator=(Option<T>&& other) noexcept {
    this->filled = other.filled;
    if (other.filled) {
      other.filled = false;
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

  T expect(const String msg) noexcept {
    if (this->isNone()) { panic(msg); }
    this->filled = false;
    const T value = std::move(this->value);
    this->dummy = 0;
    return std::move(value);
  }
  T unwrap() noexcept { return expect("Tried to unwrap an empty Option"); }
  T unwrap_or(const T or_) noexcept {
    if (this->isSome()) { return std::move(this->expect("unwrap_or is broken")); }
    return std::move(or_);
  }

  template <typename U>
  Option<U> andThen(std::function<Option<U> (const T &)> f) const noexcept {
    if (this->isSome()) {
      return std::move(f(this->value));
    }
    return Option<U>();
  }

  template <typename U>
  Option<U> map(std::function<U (const T &)> f) const noexcept {
    if (this->isSome()) {
      return std::move(Option<U>(f(this->value)));
    }
    return Option<U>();
  }
};

#endif