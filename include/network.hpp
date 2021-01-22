#ifndef IOP_NETWORK_HPP
#define IOP_NETWORK_HPP

// This hack sucks, hijackes CertStore before upstream updates
#include "certificate_storage.hpp"

#include "ESP8266HTTPClient.h"
#include "ESP8266WiFi.h"
#include "log.hpp"
#include "option.hpp"
#include "result.hpp"
#include "static_string.hpp"

typedef enum class apiStatus {
  NO_CONNECTION,
  BROKEN_SERVER,
  BROKEN_PIPE,
  TIMEOUT,

  CLIENT_BUFFER_OVERFLOW,

  OK,
  NOT_FOUND,
  FORBIDDEN,
  MUST_UPGRADE,
} ApiStatus;

class Response {
public:
  ApiStatus status;
  Option<String> payload;
  ~Response() = default;
  explicit Response(const ApiStatus status) noexcept : status(status) {}
  Response(ApiStatus status, String payload) noexcept
      : status(status), payload(std::move(payload)) {}
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept
      : status(resp.status), payload(std::move(resp.payload)) {}
  auto operator=(Response &resp) -> Response & = delete;
  auto operator=(Response &&resp) noexcept -> Response & {
    this->status = resp.status;
    this->payload = std::move(resp.payload);
    return *this;
  }
};

using RawStatus = enum class rawStatus {
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

enum HttpMethod {
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
  ~Network() = default;
  Network(const StaticString host, const LogLevel logLevel) noexcept
      : host_(host), logger(logLevel, F("NETWORK")) {}
  Network(Network const &other) = delete;
  Network(Network &&other) = delete;
  auto operator=(Network const &other) -> Network & = delete;
  auto operator=(Network &&other) -> Network & = delete;
  static auto setup() noexcept -> void;
  static auto isConnected() noexcept -> bool {
    return WiFi.status() == WL_CONNECTED;
  }
  auto wifiClient(StringView path) const noexcept
      -> Result<std::reference_wrapper<WiFiClientSecure>, RawStatus>;
  auto httpPut(StringView token, StaticString path,
               StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StringView token, StringView path,
                StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StringView token, StaticString path,
                StringView data) const noexcept -> Result<Response, int>;
  auto httpPost(StaticString path, StringView data) const noexcept
      -> Result<Response, int>;
  auto httpRequest(HttpMethod method, Option<StringView> token, StringView path,
                   Option<StringView> data) const noexcept
      -> Result<Response, int>;

  static auto rawStatusToString(RawStatus status) noexcept -> StaticString;
  auto rawStatus(int code) const noexcept -> RawStatus;

  static auto apiStatusToString(ApiStatus status) noexcept -> StaticString;
  auto apiStatus(RawStatus raw) const noexcept -> Option<ApiStatus>;

  auto host() const noexcept -> StaticString { return this->host_; };
  static void disconnect() noexcept { WiFi.disconnect(); }
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_NETWORK_DISABLED
#endif

#endif