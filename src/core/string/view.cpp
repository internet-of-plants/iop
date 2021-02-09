#include "core/string/view.hpp"
#include "core/string/cow.hpp"
#include "utils.hpp"

namespace iop {
/*
StringView::~StringView() noexcept {
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~StringView("));
  Serial.print(this->get());
  Serial.println(F(")"));
  Serial.flush();
}
*/
StringView::StringView(const UnsafeRawString &str) noexcept : str(str.get()) {}
StringView::StringView(const std::string &str) noexcept : str(str.c_str()) {}
StringView::StringView(const String &other) noexcept : str(other.c_str()) {}
StringView::StringView(const CowString &other) noexcept
    : str(other.borrow().get()) {}
auto StringView::operator==(const StringView &other) const noexcept -> bool {
  // IOP_TRACE();
  return strcmp(this->str, other.str) == 0;
}
auto StringView::operator==(const StaticString &other) const noexcept -> bool {
  // IOP_TRACE();
  return strcmp_P(this->str, other.asCharPtr()) == 0;
}
auto StringView::get() const noexcept -> const char * {
  // IOP_TRACE();
  return this->str;
}
auto StringView::length() const noexcept -> size_t {
  IOP_TRACE();
  return strlen(this->get());
}
auto StringView::isEmpty() const noexcept -> bool {
  IOP_TRACE();
  return this->length() == 0;
}

auto StringView::contains(StringView needle) const noexcept -> bool {
  IOP_TRACE();
  /*
  if (logLevel <= LogLevel::TRACE) {
    Serial.print(F("StringView(\""));
    Serial.print(this->get());
    Serial.print(F("\").contains(\""));
    Serial.print(needle.get());
    Serial.print(F("\")"));
  }
  */
  return strstr(this->get(), std::move(needle).get()) != nullptr;
}
auto StringView::contains(StaticString needle) const noexcept -> bool {
  IOP_TRACE();
  /*
  if (logLevel <= LogLevel::TRACE) {
    Serial.print(F("StringView(\""));
    Serial.print(this->get());
    Serial.print(F("\").contains(\""));
    Serial.print(needle.get());
    Serial.print(F("\")"));
  }
  */
  return strstr_P(this->get(), std::move(needle).asCharPtr()) != nullptr;
}

// FNV hash
auto StringView::hash() const noexcept -> uint64_t {
  IOP_TRACE();
  const auto *const bytes = this->get();
  const uint64_t p = 16777619; // NOLINT cppcoreguidelines-avoid-magic-numbers
  uint64_t hash = 2166136261;  // NOLINT cppcoreguidelines-avoid-magic-numbers

  const auto length = this->length();
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

auto StringView::isAllPrintable() const noexcept -> bool {
  const auto len = this->length();
  for (uint8_t index = 0; index < len; ++index) {
    const auto ch = this->str[index]; // NOLINT *-pro-bounds-pointer-arithmetic

    if (!StringView::isPrintable(ch))
      return false;
  }
  return true;
}

auto StringView::isPrintable(const char ch) noexcept -> bool {
  return ch >= 32 && ch <= 126; // NOLINT cppcoreguidelines-avoid-magic-numbers
}

auto StringView::scapeNonPrintable() const noexcept -> CowString {
  if (this->isAllPrintable())
    return CowString(*this);

  const size_t len = this->length();
  String s;
  s.reserve(len); // Scaped characters use more, but avoids the bulk of reallocs
  for (uint8_t index = 0; index < len; ++index) {
    // NOLINTNEXTLINE cppcoreguidelines-pro-bounds-pointer-arithmetic
    const auto ch = this->get()[index];
    if (StringView::isPrintable(ch)) {
      s += ch;
    } else {
      s += F("<\\");
      s += String(static_cast<uint8_t>(ch));
      s += F(">");
    }
  }
  return CowString(s);
}
} // namespace iop