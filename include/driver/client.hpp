#ifndef IOP_DRIVER_CLIENT_HPP
#define IOP_DRIVER_CLIENT_HPP

#include "driver/wifi.hpp"
#include "driver/string.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class HTTPClient;

namespace driver {
enum class RawStatus {
  CONNECTION_FAILED = -1,
  SEND_FAILED = -2,
  READ_FAILED = -3,
  ENCODING_NOT_SUPPORTED = -4,
  NO_SERVER = -5,
  READ_TIMEOUT = -6,
  CONNECTION_LOST = -7,

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
  explicit Response(const int status) noexcept: status_(status) {}
  auto status() const noexcept -> int { return this->status_; }
  auto header(iop::StaticString key) const noexcept -> std::string;
  auto await() noexcept -> Payload {
    // TODO: make it actually lazy
    return std::move(this->promise);
  }
};

class HTTPClient;

class Session {
  #ifdef IOP_DESKTOP
  int32_t fd_;
  #endif

  #ifndef IOP_NOOP
  HTTPClient *http_;
  std::unordered_map<std::string, std::string> headers;
  std::string uri_;
  #endif

  #ifdef IOP_DESKTOP
  Session(HTTPClient &http, std::string uri, int32_t fd) noexcept;
  #elif defined(IOP_ESP8266)
  Session(HTTPClient &http, std::string uri) noexcept;
  #elif defined(IOP_NOOP)
  Session() noexcept = default;
  #else
  #error "Target is not valid"
  #endif

  friend HTTPClient;

public:
  auto sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> Response;
  void addHeader(iop::StaticString key, iop::StaticString value) noexcept;
  void addHeader(iop::StaticString key, iop::StringView value) noexcept;
  void setAuthorization(std::string auth) noexcept;
  
  Session(Session& other) noexcept = delete;
  Session(Session&& other) noexcept;
  auto operator=(Session& other) noexcept -> Session & = delete;
  auto operator=(Session&& other) noexcept -> Session &;
  ~Session() noexcept;
};

class HTTPClient {
#ifdef IOP_DESKTOP
  std::vector<std::string> headersToCollect_;
#elif defined(IOP_ESP8266)
  ::HTTPClient * http;
#elif defined(IOP_NOOP)
#else
#error "Target not valid"
#endif
public:
  HTTPClient() noexcept;
  ~HTTPClient() noexcept;
  auto begin(std::string uri) noexcept -> std::optional<Session>;

  // TODO: improve this method name
  void headersToCollect(const char * headers[], size_t count) noexcept;

  friend Session;
};
}

#ifdef IOP_DESKTOP
#ifdef IOP_SSL
#error "SSL not supported for desktop right now"
#endif
#endif

#endif