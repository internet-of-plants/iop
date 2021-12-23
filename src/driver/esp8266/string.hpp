#include <string_view>
#include <WString.h>

namespace iop {
auto to_view(const String& str) -> std::string_view {
  return str.c_str();
}
}