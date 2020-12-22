#include <result.hpp>
#include <string_view.hpp>
#include <fixed_string.hpp>

// TODO: we should be using a match like api with a switch,
// to avoid else ifs that may panic with a unhelpful message on a moved result

template<typename T, typename E>
bool Result<T, E>::isEmpty() noexcept {
    return this->kind_ == EMPTY;
}

template<typename T, typename E>
void Result<T, E>::reset() noexcept {
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
}

template<typename T, typename E>
Result<T, E>& Result<T, E>::operator=(Result<T, E>&& other) noexcept {
  this->reset();
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

template<typename T, typename E>
Result<T, E>::Result(Result<T, E>&& other) noexcept {
  this->reset();
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

template<typename T, typename E>
bool Result<T, E>::isOk() const noexcept {
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

template<typename T, typename E>
bool Result<T, E>::isErr() const noexcept {
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

template<typename T, typename E>
T Result<T, E>::expectOk(const StringView msg) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at expectOk"));
  if (this->isErr()) panic_(msg);
  T value = std::move(this->ok);
  this->reset();
  return value;
}

template<typename T, typename E>
T Result<T, E>::expectOk(const StaticString msg) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at expectOk"));
  if (this->isErr()) panic_(msg);
  T value = std::move(this->ok);
  this->reset();
  return value;
}
template<typename T, typename E>
T Result<T, E>::unwrapOk() noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at unwrapOk"));
  return this->expectOk(F("Tried to unwrapOk an errored Result"));
}
template<typename T, typename E>
T Result<T, E>::unwrapOkOr(const T or_) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at unwrapOkOr"));
  if (this->isOk()) {
    const auto val = std::move(this->ok);
    this->reset();
    return val;
  } else if (this->isErr()) {
    return or_;
  }
}

template<typename T, typename E>
E Result<T, E>::expectErr(const StringView msg) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at expectErr"));
  if (this->isOk()) panic_(msg);
  const E value = std::move(this->error);
  this->reset();
  return value;
}
template<typename T, typename E>
E Result<T, E>::expectErr(const StaticString msg) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at expectErr"));
  if (this->isOk()) panic_(msg);
  const E value = std::move(this->error);
  this->reset();
  return value;
}
template<typename T, typename E>
E Result<T, E>::unwrapErr() noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at unwrapErr"));
  return this->expectErr(F("Tried to unwrapErr a Result that succeeded"));
}
template<typename T, typename E>
E Result<T, E>::unwrapErrOr(const E or_) noexcept {
  if (this->isEmpty()) panic_(F("Result is empty, at unwrapErrOr"));
  if (this->isErr()) {
    const auto val = std::move(this->err);
    this->reset();
    return val;
  } else if (this->isOk()) {
    return or_;
  }
}