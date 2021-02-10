#ifndef IOP_NETWORK_HPP
#define IOP_NETWORK_HPP

#include "core/log.hpp"
#include "core/result.hpp"

#include "ESP8266HTTPClient.h"

/// Higher level error reporting. Lower level is logged
enum class ApiStatus {
  CONNECTION_ISSUES,
  BROKEN_SERVER,
  CLIENT_BUFFER_OVERFLOW, // BROKEN_CLIENT

  OK,
  FORBIDDEN,
};

class Response;
enum class RawStatus;
enum class HttpMethod;

/// General lower level client network API, that is focused on our need.
/// Security, good error reporting, no UB possible, ergonomy.
///
/// Higher level than simple ESP8266 client network abstractions, but still
/// about http protocol
///
/// It's supposed to serve as building-block for higher level abstractions
/// Check `api.hpp`
class Network {
  iop::StaticString uri_;
  iop::Log logger;

public:
  Network(const iop::StaticString uri, const iop::LogLevel &logLevel) noexcept
      : uri_(std::move(uri)), logger(logLevel, F("NETWORK")) {
    IOP_TRACE();
  }
  auto setup() const noexcept -> void;
  auto uri() const noexcept -> iop::StaticString { return this->uri_; };

  static auto wifiClient() noexcept -> WiFiClient &;
  static void disconnect() noexcept;
  static auto isConnected() noexcept -> bool;

  auto httpPut(iop::StringView token, iop::StaticString path,
               iop::StringView data) const noexcept
      -> iop::Result<Response, int>;
  auto httpPost(iop::StringView token, iop::StringView path,
                iop::StringView data) const noexcept
      -> iop::Result<Response, int>;
  auto httpPost(iop::StringView token, iop::StaticString path,
                iop::StringView data) const noexcept
      -> iop::Result<Response, int>;
  auto httpPost(iop::StaticString path, iop::StringView data) const noexcept
      -> iop::Result<Response, int>;

  auto httpRequest(HttpMethod method, const iop::Option<iop::StringView> &token,
                   iop::StringView path,
                   const iop::Option<iop::StringView> &data) const noexcept
      -> iop::Result<Response, int>;

  static auto rawStatusToString(const RawStatus &status) noexcept
      -> iop::StaticString;
  auto rawStatus(int code) const noexcept -> RawStatus;

  static auto apiStatusToString(const ApiStatus &status) noexcept
      -> iop::StaticString;
  auto apiStatus(const RawStatus &raw) const noexcept -> iop::Option<ApiStatus>;

  ~Network() noexcept { IOP_TRACE(); }
  Network(Network const &other) : uri_(other.uri_), logger(other.logger) {
    IOP_TRACE();
  }
  Network(Network &&other) = delete;
  auto operator=(Network const &other) -> Network & {
    IOP_TRACE();
    if (this == &other)
      return *this;
    this->uri_ = other.uri_;
    this->logger = other.logger;
    return *this;
  }
  auto operator=(Network &&other) -> Network & = delete;
};

class Response {
public:
  ApiStatus status;
  iop::Option<String> payload;
  ~Response() noexcept;
  explicit Response(const ApiStatus &status) noexcept;
  Response(const ApiStatus &status, String payload) noexcept;
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept;
  auto operator=(Response &resp) noexcept -> Response & = delete;
  auto operator=(Response &&resp) noexcept -> Response &;
};

enum class HttpMethod {
  GET,
  HEAD,
  POST,
  PUT,
  PATCH,
  DELETE,
  CONNECT,
  OPTIONS,
};

enum class RawStatus {
  CONNECTION_FAILED = HTTPC_ERROR_CONNECTION_FAILED,
  SEND_FAILED = HTTPC_ERROR_SEND_PAYLOAD_FAILED,
  READ_FAILED = HTTPC_ERROR_STREAM_WRITE,
  ENCODING_NOT_SUPPORTED = HTTPC_ERROR_ENCODING,
  NO_SERVER = HTTPC_ERROR_NO_HTTP_SERVER,
  READ_TIMEOUT = HTTPC_ERROR_READ_TIMEOUT,
  CONNECTION_LOST = HTTPC_ERROR_CONNECTION_LOST,

  OK = HTTP_CODE_OK,
  SERVER_ERROR = HTTP_CODE_INTERNAL_SERVER_ERROR,
  FORBIDDEN = HTTP_CODE_FORBIDDEN,

  UNKNOWN = 999
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_NETWORK_DISABLED
#endif

#endif