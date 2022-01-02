#include "driver/string.hpp"

namespace iop {
auto StaticString::toString() const noexcept -> std::string { return ""; }
auto StaticString::length() const noexcept -> size_t { return 0; }
}