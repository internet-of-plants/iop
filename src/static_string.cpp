#include "static_string.hpp"

#include "configuration.h"
#include "string_view.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

StaticString::~StaticString() {
  IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~StaticString("));
  Serial.print(this->get());
  Serial.println(F(")"));
  Serial.flush();
}

// NOLINTNEXTLINE hicpp-explicit-conversions
StaticString::StaticString(const __FlashStringHelper *str) noexcept : str(str) {
  IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~StaticString("));
  Serial.print(this->get());
  Serial.println(F(")"));
  Serial.flush();
}
StaticString::StaticString(StaticString const &other) noexcept
    : str(other.str) {
  // IOP_TRACE();
}
StaticString::StaticString(StaticString &&other) noexcept : str(other.str) {
  // IOP_TRACE();
}
auto StaticString::operator=(StaticString const &other) noexcept
    -> StaticString & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
}
auto StaticString::operator=(StaticString &&other) noexcept -> StaticString & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
}
auto StaticString::get() const noexcept -> const __FlashStringHelper * {
  // IOP_TRACE();
  return this->str;
}
auto StaticString::contains(const StringView needle) const noexcept -> bool {
  IOP_TRACE();
  return strstr(String(this->get()).c_str(), needle.get()) != nullptr;
}
auto StaticString::contains(const StaticString needle) const noexcept -> bool {
  IOP_TRACE();
  return strstr_P(this->asCharPtr(), needle.asCharPtr()) != nullptr;
}
auto StaticString::length() const noexcept -> size_t {
  IOP_TRACE();
  return strlen_P(this->asCharPtr());
}
auto StaticString::isEmpty() const noexcept -> bool {
  IOP_TRACE();
  return this->length() == 0;
}
// Be careful when calling this function, if you pass PGM_P to a function that
// expects a regular char* a hardware exception may happen, PROGMEM data needs
// to be read in 32 bits alignment, this has caused trouble in tha past and
// may still do It's why StringView doesn't have a constructor for
// StaticString
auto StaticString::asCharPtr() const noexcept -> PGM_P {
  IOP_TRACE();
  // NOLINT *-pro-type-reinterpret-cast
  return reinterpret_cast<PGM_P>(this->get());
}