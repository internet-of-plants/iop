#include "core/string.hpp"
#include "core/log.hpp"
#include "core/log.hpp"
#include "core/utils.hpp"
#include <string>

namespace iop {
auto hashString(const std::string_view txt) noexcept -> uint64_t {
  IOP_TRACE();
  const auto *const bytes = txt.begin();
  const uint64_t p = 16777619; // NOLINT cppcoreguidelines-avoid-magic-numbers
  uint64_t hash = 2166136261;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  const auto length = txt.length();
  for (uint32_t i = 0; i < length; ++i) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    hash = (hash ^ (uint64_t)bytes[i]) * p;
  }

  hash += hash << 13; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 7;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 3;  // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash ^= hash >> 17; // NOLINT cppcoreguidelines-avoid-magic-numbers
  hash += hash << 5;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  return hash;
}

auto isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126; // NOLINT cppcoreguidelines-avoid-magic-numbers
}

auto isAllPrintable(const std::string_view txt) noexcept -> bool {
  const auto len = txt.length();
  for (uint8_t index = 0; index < len; ++index) {
    const auto ch = txt.begin()[index]; // NOLINT *-pro-bounds-pointer-arithmetic

    if (!isPrintable(ch))
      return false;
  }
  return true;
}

auto scapeNonPrintable(const std::string_view txt) noexcept -> CowString {
  if (isAllPrintable(txt))
    return CowString(txt);

  const size_t len = txt.length();
  std::string s(len, '\0');
  for (uint8_t index = 0; index < len; ++index) {
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    const auto ch = txt.begin()[index];
    if (isPrintable(ch)) {
      s += ch;
    } else {
      s += "<\\";
      s += std::to_string(static_cast<uint8_t>(ch));
      s += ">";
    }
  }
  return CowString(s);
}

auto to_view(const std::string& str) -> std::string_view {
  return str.c_str();
}
#ifndef IOP_DESKTOP 
auto to_view(const String& str) -> std::string_view {
  return str.c_str();
}
#endif
auto to_view(const CowString& str) -> std::string_view {
  if (iop::is_ok(str)) {
    return iop::unwrap_ok_ref(str, IOP_CTX());
  }
  return iop::unwrap_err_ref(str, IOP_CTX());
}

// NOLINTNEXTLINE hicpp-explicit-conversions
StaticString::StaticString(const __FlashStringHelper *str) noexcept : str(str) {}
StaticString::StaticString(StaticString &&other) noexcept: str(other.str) {}
auto StaticString::get() const noexcept -> const __FlashStringHelper * {
  // IOP_TRACE();
  return this->str;
}
auto StaticString::contains(std::string_view needle) const noexcept -> bool {
  if (Log::isTracing()) {
    Log::print(F("StaticString(\""), iop::LogLevel::TRACE, iop::LogType::START);
    Log::print(*this, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(F("\").contains(std::string_view(\""), iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(needle, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(F("\"))"), iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
  }
  std::string msg(this->length(), '\0');
  memmove_P(&msg.front(), this->asCharPtr(), this->length());
  return strstr(msg.c_str(), std::move(needle).begin()) !=
         nullptr;
}
auto StaticString::contains(StaticString needle) const noexcept -> bool {
  if (Log::isTracing()) {
    Log::print(F("StaticString(\""), iop::LogLevel::TRACE, iop::LogType::START);
    Log::print(*this, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(F("\").contains(StaticString(\""), iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(needle, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
    Log::print(F("\"))"), iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
  }
  return strstr_P(this->toStdString().c_str(), std::move(needle).asCharPtr()) != nullptr;
}
auto StaticString::length() const noexcept -> size_t {
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