#include "core/string/static.hpp"
#include "core/tracer.hpp"
#include <string>

namespace iop {
/*
StaticString::~StaticString() noexcept {
  IOP_TRACE();
  if (!Log::isTracing())
    return;
  Log::print(F("~StaticString("));
  Log::print(this->get());
  Log::print(F(")\n"));
  Log::flush();
}
*/

// NOLINTNEXTLINE hicpp-explicit-conversions
StaticString::StaticString(const __FlashStringHelper *str) noexcept : str(str) {
  /*
  IOP_TRACE();
  if (!Log::isTracing())
    return;
  Log::print(F("~StaticString("));
  Log::print(this->get());
  Log::print(F(")\n"));
  Log::flush();
  */
}
auto StaticString::get() const noexcept -> const __FlashStringHelper * {
  // IOP_TRACE();
  return this->str;
}
auto StaticString::contains(std::string_view needle) const noexcept -> bool {
  IOP_TRACE();
  /*
  if (!Log::isTracing()) {
    Log::print(F("StaticString(\""));
    Log::print(this->get());
    Log::print(F("\").contains(\""));
    Log::print(needle.get());
    Log::print(F("\")"));
  }
  */
  std::string msg(this->length(), '\0');
  memmove_P(&msg.front(), this->asCharPtr(), this->length());
  return strstr(msg.c_str(), std::move(needle).begin()) !=
         nullptr;
}
auto StaticString::contains(StaticString needle) const noexcept -> bool {
  IOP_TRACE();
  /*
  if (!Log::isTracing()) {
    Log::print(F("StaticString(\""));
    Log::print(this->get());
    Log::print(F("\").contains(StaticString(\""));
    Log::print(needle.get());
    Log::print(F("\"))"));
  }
  */
  return strstr_P(this->asCharPtr(), std::move(needle).asCharPtr()) != nullptr;
}
auto StaticString::length() const noexcept -> size_t {
  //IOP_TRACE();
  return strlen_P(this->asCharPtr());
}

auto StaticString::toStdString() const noexcept -> std::string {
  const auto len = this->length();
  std::string msg(len, '\0');
  memmove_P(&msg.front(), this->asCharPtr(), len);
  return msg;
}

auto StaticString::isEmpty() const noexcept -> bool {
  IOP_TRACE();
  return this->length() == 0;
}
// Be careful when calling this function, if you pass PGM_P to a function that
// expects a regular char* a hardware exception may happen, PROGMEM data needs
// to be read in 32 bits alignment, this has caused trouble in the past
auto StaticString::asCharPtr() const noexcept -> PGM_P {
  //IOP_TRACE();
  // NOLINT *-pro-type-reinterpret-cast
  return reinterpret_cast<PGM_P>(this->get());
}
} // namespace iop