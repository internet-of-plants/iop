#include "driver/string.hpp"
#include "pgmspace.h"

namespace iop {
auto StaticString::toString() const noexcept -> std::string {
  const auto len = this->length();
  std::string msg(len, '\0');
  memcpy_P(&msg.front(), this->asCharPtr(), len);
  return msg;
}
auto StaticString::length() const noexcept -> size_t { return strlen_P(this->asCharPtr()); }
}