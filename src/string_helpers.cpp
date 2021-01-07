#include "static_string.hpp"
#include "string_view.hpp"
#include "unsafe_raw_string.hpp"


StringView UnsafeRawString::operator*() const noexcept { return *this; }
StringView UnsafeRawString::operator->() const noexcept { return *this; }
bool StaticString::contains(const StaticString needle) const noexcept {
  return StringView(*this).contains(needle);
}
bool StaticString::contains(const StringView needle) const noexcept {
  return StringView(*this).contains(needle);
}