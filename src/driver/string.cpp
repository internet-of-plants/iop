#include "driver/string.hpp"
#include "core/panic.hpp"

#ifndef IOP_DESKTOP
#include <WString.h>
#endif

namespace iop {
#ifndef IOP_DESKTOP
auto to_view(const String& str) -> std::string_view {
  return str.c_str();
}
#endif

auto to_view(const std::string& str) -> std::string_view {
  return str.c_str();
}

auto to_view(const CowString& str) -> std::string_view {
  if (const auto *value = std::get_if<std::string_view>(&str)) {
    return *value;
  } else if (const auto *value = std::get_if<std::string>(&str)) {
    return iop::to_view(*value);
  }
  iop_panic(FLASH("Invalid variant types"));
}
} // namespace iop
