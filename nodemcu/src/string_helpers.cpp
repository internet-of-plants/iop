#include <unsafe_raw_string.hpp>
#include <string_view.hpp>

StringView UnsafeRawString::operator*() const noexcept {
    return StringView(*this);
}

StringView UnsafeRawString::operator->() const noexcept {
    return StringView(*this);
}