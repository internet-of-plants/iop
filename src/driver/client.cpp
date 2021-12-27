#ifdef IOP_DESKTOP
#include "driver/desktop/client.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/client.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/client.hpp"
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
    return FLASH("CONNECTION_FAILED");
  case driver::RawStatus::SEND_FAILED:
    return FLASH("SEND_FAILED");
  case driver::RawStatus::READ_FAILED:
    return FLASH("READ_FAILED");
  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
    return FLASH("ENCODING_NOT_SUPPORTED");
  case driver::RawStatus::NO_SERVER:
    return FLASH("NO_SERVER");
  case driver::RawStatus::READ_TIMEOUT:
    return FLASH("READ_TIMEOUT");
  case driver::RawStatus::CONNECTION_LOST:
    return FLASH("CONNECTION_LOST");
  case driver::RawStatus::OK:
    return FLASH("OK");
  case driver::RawStatus::SERVER_ERROR:
    return FLASH("SERVER_ERROR");
  case driver::RawStatus::FORBIDDEN:
    return FLASH("FORBIDDEN");
  case driver::RawStatus::UNKNOWN:
    return FLASH("UNKNOWN");
  }
  return FLASH("UNKNOWN-not-documented");
}
}

namespace iop {
  Data data;
}