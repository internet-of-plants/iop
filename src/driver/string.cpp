#ifndef IOP_DESKTOP
#include "driver/esp8266/string.hpp"
#endif

#include "core/panic.hpp"

namespace iop {
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
