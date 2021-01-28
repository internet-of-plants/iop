#ifndef IOP_NETWORK_HPP
#define IOP_NETWORK_HPP

// This hack sucks, hijackes CertStore before upstream updates
#include "certificate_storage.hpp"

#include "log.hpp"
#include "option.hpp"
#include "result.hpp"
#include "static_string.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"

enum class ApiStatus {
  NO_CONNECTION,
  BROKEN_SERVER,
  BROKEN_PIPE,
  TIMEOUT,

  CLIENT_BUFFER_OVERFLOW,

  OK,
  NOT_FOUND,
  FORBIDDEN,
  MUST_UPGRADE,
};

class Response {
public:
  ApiStatus status;
  Option<String> payload;
  ~Response();
  explicit Response(const ApiStatus status) noexcept;
  Response(ApiStatus status, String payload) noexcept;
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept;
  auto operator=(Response &resp) -> Response & = delete;
  auto operator=(Response &&resp) noexcept -> Response &;
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
  NOT_FOUND = HTTP_CODE_NOT_FOUND,
  SERVER_ERROR = HTTP_CODE_INTERNAL_SERVER_ERROR,
  FORBIDDEN = HTTP_CODE_FORBIDDEN,
  BAD_REQUEST = HTTP_CODE_BAD_REQUEST,
  MUST_UPGRADE = HTTP_CODE_PRECONDITION_FAILED,

  UNKNOWN = 999
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

/// Try to make the ESP8266 network API more pallatable for our needs
class Network {
  StaticString host_;
  Log logger;

public:
  ~Network() { IOP_TRACE(); }
  Network(const StaticString host, const LogLevel logLevel) noexcept
      : host_(host), logger(logLevel, F("NETWORK")) {
    IOP_TRACE();
  }
  Network(Network const &other) : host_(other.host_), logger(other.logger) {
    IOP_TRACE();
  }
  Network(Network &&other) = delete;
  auto operator=(Network const &other) -> Network & {
    IOP_TRACE();
    this->host_ = other.host_;
    this->logger = other.logger;
    return *this;
  }
  auto operator=(Network &&other) -> Network & = delete;
  auto setup() const noexcept -> void;
  static auto isConnected() noexcept -> bool;
  auto httpPut(StringView token, StaticString path,
               StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StringView token, StringView path,
                StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StringView token, StaticString path,
                StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StaticString path, StringView data) const noexcept
      -> Result<Response, int>;
  auto httpRequest(HttpMethod method, const Option<StringView> &token,
                   StringView path,
                   const Option<StringView> &data) const noexcept
      -> Result<Response, int>;

  static auto wifiClient() noexcept -> WiFiClient &;
  static auto rawStatusToString(RawStatus status) noexcept -> StaticString;
  auto rawStatus(int code) const noexcept -> RawStatus;

  static auto apiStatusToString(ApiStatus status) noexcept -> StaticString;
  auto apiStatus(RawStatus raw) const noexcept -> Option<ApiStatus>;

  auto host() const noexcept -> StaticString {
    IOP_TRACE();
    return this->host_;
  };
  static void disconnect() noexcept;
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_NETWORK_DISABLED
#endif

#endif