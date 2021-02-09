#include "core/string/unsafe_raw.hpp"
#include "core/string/view.hpp"

namespace iop {
auto UnsafeRawString::operator*() const noexcept -> StringView {
  // IOP_TRACE();
  return *this;
}
auto UnsafeRawString::operator->() const noexcept -> StringView {
  // IOP_TRACE();
  return *this;
}

/*
UnsafeRawString::~UnsafeRawString() noexcept {
  // IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~UnsafeRawString("));
  Serial.print(this->str);
  Serial.println(F(")"));
  Serial.flush();
}
*/

UnsafeRawString::UnsafeRawString(const char *str) noexcept : str(str) {
  /*
  IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("UnsafeRawString("));
  Serial.print(this->str);
  Serial.println(F(")"));
  Serial.flush();
  */
}

/*
UnsafeRawString::UnsafeRawString(const UnsafeRawString &str) noexcept
    : str(str.str) {
  // IOP_TRACE();
}

UnsafeRawString::UnsafeRawString(UnsafeRawString &&str) noexcept
    : str(str.str) {
  // IOP_TRACE();
}
*/

// NOLINTNEXTLINE cert-oop54-cpp
auto UnsafeRawString::operator=(UnsafeRawString const &other) noexcept
    -> UnsafeRawString & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
}
auto UnsafeRawString::operator=(UnsafeRawString &&other) noexcept
    -> UnsafeRawString & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
}
auto UnsafeRawString::get() const noexcept -> const char * {
  /*
  IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return this->str;
  Serial.print(F("UnsafeRawString("));
  Serial.print(this->str);
  Serial.println(F(")"));
  Serial.flush();
  */
  return this->str;
}
} // namespace iop