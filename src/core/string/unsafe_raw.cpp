#include "core/string/unsafe_raw.hpp"
#include "core/string/view.hpp"

namespace iop {
auto UnsafeRawString::operator*() const noexcept -> StringView {
  // IOP_TRACE();
  return *this;
}

/*
UnsafeRawString::~UnsafeRawString() noexcept {
  // IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Log::print(F("~UnsafeRawString("));
  Log::print(this->str);
  Log::print(F(")\n"));
  Log::flush();
}
*/

UnsafeRawString::UnsafeRawString(const char *str) noexcept : str(str) {
  /*
  IOP_TRACE();
  if (logLevel > LogLevel::TRACE)
    return;
  Log::print(F("UnsafeRawString("));
  Log::print(this->str);
  Log::print(F(")\n"));
  Log::flush();
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
  Log::print(F("UnsafeRawString("));
  Log::print(this->str);
  Log::print(F(")\n"));
  Log::flush();
  */
  return this->str;
}
} // namespace iop