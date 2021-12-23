#ifdef IOP_DESKTOP
#include "driver/desktop/client.hpp"
#else
#include "driver/esp8266/client.hpp"
#endif

namespace driver {
auto rawStatus(const int code) noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;
  case HTTPC_ERROR_CONNECTION_FAILED:
    return RawStatus::CONNECTION_FAILED;
  case HTTPC_ERROR_SEND_HEADER_FAILED:
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
    return RawStatus::SEND_FAILED;
  case HTTPC_ERROR_NOT_CONNECTED:
  case HTTPC_ERROR_CONNECTION_LOST:
    return RawStatus::CONNECTION_LOST;
  case HTTPC_ERROR_NO_STREAM:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_NO_HTTP_SERVER:
    return RawStatus::NO_SERVER;
  case HTTPC_ERROR_ENCODING:
    // Unsupported Transfer-Encoding header, if set it must be "chunked"
    return RawStatus::ENCODING_NOT_SUPPORTED;
  case HTTPC_ERROR_STREAM_WRITE:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_READ_TIMEOUT:
    return RawStatus::READ_TIMEOUT;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

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