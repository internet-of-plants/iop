#ifndef IOP_NETWORK_H
#define IOP_NETWORK_H

// TODO: Remove. It's here so we can hijack the CertStore from BearSSL
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
  Response(const ApiStatus status) noexcept : status(status) {}
  Response(const ApiStatus status, const String payload) noexcept
      : status(status), payload(std::move(payload)) {}
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept
      : status(resp.status), payload(std::move(resp.payload)) {}
  Response &operator=(Response &resp) = delete;
  Response &operator=(Response &&resp) noexcept {
    this->status = std::move(resp.status);
    this->payload = std::move(resp.payload);
    return *this;
  }
};

typedef enum class rawStatus {
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
} RawStatus;

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
  Network(const StaticString host, const LogLevel logLevel) noexcept
      : host_(host), logger(logLevel, F("NETWORK")) {}
  Network(Network &other) = delete;
  Network(Network &&other) = delete;
  Network &operator=(Network &other) = delete;
  Network &operator=(Network &&other) = delete;
  void setup() const noexcept;
  bool isConnected() const noexcept;
  Result<std::shared_ptr<HTTPClient>, RawStatus>
  httpClient(const StaticString path,
             const Option<StringView> &token) const noexcept;
  Result<Response, int> httpPut(const StringView token, const StaticString path,
                                const StringView data) const noexcept;
  Result<Response, int> httpPost(const StringView token,
                                 const StaticString path,
                                 const StringView data) const noexcept;
  Result<Response, int> httpPost(const StaticString path,
                                 const StringView data) const noexcept;
  Result<Response, int>
  httpRequest(const HttpMethod method, const Option<StringView> &token,
              const StaticString path,
              const Option<StringView> &data) const noexcept;

  StaticString rawStatusToString(const RawStatus status) const noexcept;
  RawStatus rawStatus(const int code) const noexcept;

  StaticString apiStatusToString(const ApiStatus status) const noexcept;
  Option<ApiStatus> apiStatus(const RawStatus raw) const noexcept;

  String macAddress() const noexcept;
  StaticString host() const noexcept { return this->host_; };
  void disconnect() const noexcept;
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_NETWORK_DISABLED
#endif

#endif