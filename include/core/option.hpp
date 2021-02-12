#ifndef IOP_CORE_OPTION_HPP
#define IOP_CORE_OPTION_HPP

#include "core/memory.hpp" // Used for enable_if_t, for now
#include "core/panic.hpp"
#include "core/string/view.hpp"
#include "core/tracer.hpp"

#include <functional>

#define UNWRAP(opt)                                                            \
  std::move((opt).expect(F(#opt " is None"), IOP_CODE_POINT()))
#define UNWRAP_REF(opt) UNWRAP((opt).asRef()).get()
#define UNWRAP_MUT(opt) UNWRAP((opt).asMut()).get()

namespace iop {

/// Optional sum-type, may be filled and contain a T, or not be filled.
///
/// Trying to access data that is not there triggers `iop_panic`. Check before
///
/// Most methods move out by default. You probably want to call `.asRef()` or
/// `.asMut()` before calling those methods. Or use `UNWRAP_{REF,MUT}`
///
/// Exceptions in T's destructor will trigger abort
///
/// T cannot be a reference, it won't compile. Use std::reference_wrapper<T>
template <typename T, typename = enable_if_t<!std::is_reference<T>::value>>
// Weird ass false positive
// NOLINTNEXTLINE *-special-member-functions
class Option {
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
  explicit Option(T v) noexcept : filled(true), value(std::move(v)) {
    IOP_TRACE();
  }

  // We don't support initializer list based emplace, but feel free to add
  template <class... Args> void emplace(Args &&...args) {
    IOP_TRACE();
    this->filled = true;
    this->value = std::move(T(std::forward<Args>(args)...));
  }

  auto asMut() noexcept -> Option<std::reference_wrapper<T>> {
    IOP_TRACE();
    if (this->isSome()) {
      return Option<std::reference_wrapper<T>>(std::ref(this->value));
    }
    return Option<std::reference_wrapper<T>>();
  }

  auto asRef() const noexcept -> Option<std::reference_wrapper<const T>> {
    IOP_TRACE();
    if (this->isSome()) {
      return Option<std::reference_wrapper<const T>>(std::ref(this->value));
    }
    return Option<std::reference_wrapper<const T>>();
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
    return std::move(other);
  }

  auto unwrapOr(T &&or_) noexcept -> T {
    IOP_TRACE();
    if (this->isSome())
      return UNWRAP(*this);
    return std::move(or_);
  }

  template <typename U>
  auto andThen(std::function<Option<U>(const T)> &f) noexcept -> Option<U> {
    IOP_TRACE();
    if (this->isSome())
      return f(UNWRAP(*this));
    return Option<U>();
  }

  template <typename U>
  auto map(std::function<U(const T)> &f) noexcept -> Option<U> {
    IOP_TRACE();
    if (this->isSome())
      return f(UNWRAP(*this));
    return Option<U>();
  }

  auto or_(Option<T> &&v) noexcept -> Option<T> {
    IOP_TRACE();
    if (this->isSome())
      return std::move(this);
    return std::move(v);
  }

  auto orElse(std::function<Option<T>()> &v) noexcept -> Option<T> {
    IOP_TRACE();
    if (this->isSome())
      return std::move(this);
    return v();
  }

  ~Option() noexcept {
    IOP_TRACE();
    this->reset();
  }

  Option(Option<T> const &other) = delete;
  // Weird ass false positive
  // NOLINTNEXTLINE google-explicit-constructor hicpp-explicit-conversions
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
  auto operator=(Option<T> const &other) noexcept -> Option<T> & = delete;
  // Weird ass false positive
  // NOLINTNEXTLINE *-copy-assignment-signature *-unconventional-assign-operator
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

  // Allows for more detailed panics, used by UNWRAP(_REF, _MUT) macros
  auto expect(StringView msg, const CodePoint point) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      iop::panicHandler(std::move(msg), point);

    T value = std::move(this->value);
    this->reset();
    return std::move(value);
  }
  auto expect(StaticString msg, const CodePoint point) noexcept -> T {
    IOP_TRACE();
    if (this->isNone())
      iop::panicHandler(std::move(msg), point);

    T value = std::move(this->value);
    this->reset();
    return std::move(value);
  }
};

template <typename T> auto some(T value) noexcept -> Option<T> {
  return Option<T>(std::move(value));
}

} // namespace iop

#define MAYBE_PROGMEM_STRING_EMPTY(name)                                       \
  static const iop::Option<iop::StaticString> name;
#define MAYBE_PROGMEM_STRING(name, msg)                                        \
  PROGMEM_STRING(name_##storage, msg);                                         \
  static const iop::Option<iop::StaticString> name(name_##storage);

#endif