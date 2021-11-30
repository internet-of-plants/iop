#ifndef IOP_DRIVER_CLIENT
#define IOP_DRIVER_CLIENT

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include "driver/wifi.hpp"
#include "core/string.hpp"

#ifndef IOP_DESKTOP
// TODO: remove this import from here, but... how?
#include "ESP8266HTTPClient.h"
#endif

#ifdef IOP_DESKTOP
#define HTTPC_ERROR_CONNECTION_FAILED   (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)
#endif

namespace driver {
enum class RawStatus {
  CONNECTION_FAILED = HTTPC_ERROR_CONNECTION_FAILED,
  SEND_FAILED = HTTPC_ERROR_SEND_PAYLOAD_FAILED,
  READ_FAILED = HTTPC_ERROR_STREAM_WRITE,
  ENCODING_NOT_SUPPORTED = HTTPC_ERROR_ENCODING,
  NO_SERVER = HTTPC_ERROR_NO_HTTP_SERVER,
  READ_TIMEOUT = HTTPC_ERROR_READ_TIMEOUT,
  CONNECTION_LOST = HTTPC_ERROR_CONNECTION_LOST,

  OK = 200,
  SERVER_ERROR = 500,
  FORBIDDEN = 403,

  UNKNOWN = 999
};

auto rawStatus(int code) noexcept -> RawStatus;
auto rawStatusToString(RawStatus status) noexcept -> iop::StaticString;

class Payload {
public:
  std::string payload;
  Payload() noexcept: payload("") {}
  explicit Payload(std::string data) noexcept: payload(std::move(data)) {}
};

class Response {
  std::unordered_map<std::string, std::string> headers_;
  Payload promise;
  int status_;

public:
  Response(std::unordered_map<std::string, std::string> headers, const Payload payload, const int status) noexcept: headers_(headers), promise(payload), status_(status) {}
  explicit Response(const RawStatus status) noexcept: status_(static_cast<uint8_t>(status)) {}
  explicit Response(const int status) noexcept: status_(status) {}
  auto status() const noexcept { return this->status_; }
  auto header(iop::StaticString key) const noexcept -> std::string;
  auto await() noexcept -> Payload {
    // TODO: make it actually lazy
    return std::move(this->promise);
  }
};

class HTTPClient;

class Session {
  HTTPClient * http_;
  std::unordered_map<std::string, std::string> headers;
  std::string uri_;

  #ifdef IOP_DESKTOP
  int32_t fd_;
  #endif

public:
  auto sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> std::variant<Response, int>;
  void addHeader(iop::StaticString key, iop::StaticString value) noexcept;
  void addHeader(iop::StaticString key, std::string_view value) noexcept;
  void setAuthorization(std::string auth) noexcept;
  #ifdef IOP_DESKTOP
  Session(HTTPClient &http, std::string uri, int32_t fd) noexcept;
  #else
  Session(HTTPClient &http, std::string uri) noexcept;
  #endif
  
  Session(Session& other) noexcept = delete;
  Session(Session&& other) noexcept;
  auto operator=(Session& other) noexcept = delete;
  auto operator=(Session&& other) noexcept;
  ~Session() noexcept;
};

class HTTPClient {
#ifdef IOP_DESKTOP
  std::vector<iop::StaticString> headersToCollect_;
#else
  ::HTTPClient http;
#endif
public:
  HTTPClient() = default;
  auto begin(std::string uri) noexcept -> std::optional<Session>;

  // TODO: improve this method name
  void headersToCollect(std::vector<iop::StaticString> headers) noexcept;

  friend Session;
};
}

#ifdef IOP_DESKTOP
#ifdef IOP_SSL
#error "SSL not supported for desktop right now"
#endif
#endif

#endif