#include "static_string.hpp"
#include "string_view.hpp"
#include "unsafe_raw_string.hpp"

auto UnsafeRawString::operator*() const noexcept -> StringView { return *this; }
auto UnsafeRawString::operator->() const noexcept -> StringView {
  return *this;
}
auto StaticString::contains(const StaticString needle) const noexcept -> bool {
  return StringView(*this).contains(needle);
}
auto StaticString::contains(const StringView needle) const noexcept -> bool {
  return StringView(*this).contains(needle);
}