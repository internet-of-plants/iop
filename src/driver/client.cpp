#ifdef IOP_DESKTOP
#include "driver/desktop/client.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/client.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/client.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/client.hpp"
#else
#error "Target not supported"
#endif

#include "driver/log.hpp"
#include "driver/network.hpp"

namespace driver {
auto rawStatusToString(const RawStatus status) noexcept -> iop::StaticString {
  IOP_TRACE();
  switch (status) {
  case driver::RawStatus::CONNECTION_FAILED:
    return IOP_STATIC_STRING("CONNECTION_FAILED");
  case driver::RawStatus::SEND_FAILED:
    return IOP_STATIC_STRING("SEND_FAILED");
  case driver::RawStatus::READ_FAILED:
    return IOP_STATIC_STRING("READ_FAILED");
  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
    return IOP_STATIC_STRING("ENCODING_NOT_SUPPORTED");
  case driver::RawStatus::NO_SERVER:
    return IOP_STATIC_STRING("NO_SERVER");
  case driver::RawStatus::READ_TIMEOUT:
    return IOP_STATIC_STRING("READ_TIMEOUT");
  case driver::RawStatus::CONNECTION_LOST:
    return IOP_STATIC_STRING("CONNECTION_LOST");
  case driver::RawStatus::OK:
    return IOP_STATIC_STRING("OK");
  case driver::RawStatus::SERVER_ERROR:
    return IOP_STATIC_STRING("SERVER_ERROR");
  case driver::RawStatus::FORBIDDEN:
    return IOP_STATIC_STRING("FORBIDDEN");
  case driver::RawStatus::UNKNOWN:
    return IOP_STATIC_STRING("UNKNOWN");
  }
  return IOP_STATIC_STRING("UNKNOWN-not-documented");
}
}

namespace iop {
  Data data;
}