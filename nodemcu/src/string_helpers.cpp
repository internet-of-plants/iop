#include <string_view.hpp>
#include <unsafe_raw_string.hpp>

StringView UnsafeRawString::operator*() const noexcept {
  return StringView(*this);
}

StringView UnsafeRawString::operator->() const noexcept {
  return StringView(*this);
}