#include "string_view.hpp"

#include "configuration.h"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

StringView::~StringView() {
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~StringView("));
  Serial.print(this->get());
  Serial.println(F(")"));
  Serial.flush();
}
StringView::StringView(const UnsafeRawString str) noexcept : str(str.get()) {}

// NOLINTNEXTLINE hicpp-explicit-conversions
StringView::StringView(const std::string &str) noexcept : str(str.c_str()) {
  IOP_TRACE();
}
// NOLINTNEXTLINE hicpp-explicit-conversions
StringView::StringView(const String &other) noexcept : str(other.c_str()) {
  IOP_TRACE();
}
StringView::StringView(const StringView &other) noexcept : str(other.str) {
  // IOP_TRACE();
}
StringView::StringView(StringView &&other) noexcept : str(other.str) {
  // IOP_TRACE();
}
auto StringView::operator==(const StringView &other) const noexcept -> bool {
  IOP_TRACE();
  return strcmp(this->str, other.str) == 0;
}
auto StringView::operator==(const StaticString &other) const noexcept -> bool {
  IOP_TRACE();
  return strcmp_P(this->str, other.asCharPtr()) == 0;
}
auto StringView::operator=(StringView const &other) noexcept -> StringView & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
}
auto StringView::operator=(StringView &&other) noexcept -> StringView & {
  // IOP_TRACE();
  this->str = other.str;
  return *this;
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

auto StringView::contains(const StringView needle) const noexcept -> bool {
  IOP_TRACE();
  return strstr(this->get(), needle.get()) != nullptr;
}

auto StringView::contains(const StaticString needle) const noexcept -> bool {
  IOP_TRACE();
  return strstr_P(this->get(), needle.asCharPtr()) != nullptr;
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