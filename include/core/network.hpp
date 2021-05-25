#ifndef IOP_NETWORK_HPP
#define IOP_NETWORK_HPP

#include "core/cert_store.hpp"
#include "core/log.hpp"
#include "core/result.hpp"

#ifdef IOP_DESKTOP
#include "driver/client.hpp"
class WiFiClient;
#else
#include "ESP8266HTTPClient.h"
#endif

namespace iop {
/// Higher level error reporting. Lower level is logged
enum class NetworkStatus {
  CONNECTION_ISSUES,
  BROKEN_SERVER,
  CLIENT_BUFFER_OVERFLOW, // BROKEN_CLIENT

  OK,
  FORBIDDEN,
};

class Response;
enum class RawStatus;
enum class HttpMethod;

/// If server set LAST_VERSION HTTP header is different than current sketch's
/// md5 hash the hook is called
class UpgradeHook {
public:
  using UpgradeScheduler = std::function<void()>;

  UpgradeScheduler schedule;

  // Noop
  static void defaultHook() noexcept;

  explicit UpgradeHook(UpgradeScheduler scheduler) noexcept
      : schedule(std::move(scheduler)) {
    IOP_TRACE();
  }
};

/// General lower level client network API, that is focused on our need.
/// Security, good error reporting, no UB possible, ergonomy.
///
/// Higher level than simple ESP8266 client network abstractions, but still
/// about http protocol
///
/// MUST CALL `setCertStore` otherwise TLS won't work. It _will_ panic.
///
/// `setUpgradeHook` to monitor when the server offers upgrade
///
/// Check `api.hpp` for even higher level code
class Network {
  StaticString uri_;
  Log logger;

public:
  Network(StaticString uri, const LogLevel &logLevel) noexcept
      : uri_(std::move(uri)), logger(logLevel, F("NETWORK")) {
    IOP_TRACE();
  }
  auto setup() const noexcept -> void;
  auto uri() const noexcept -> StaticString { return this->uri_; };

  static void setCertStore(CertStore store) noexcept;

  /// Replaces current hook for this. Very useful to support upgrades
  /// reported by the network (LAST_VERSION header different than current
  /// sketch hash) Default is a noop
  static void setUpgradeHook(UpgradeHook scheduler) noexcept;
  /// Removes current hook, replaces for default one (noop)
  static auto takeUpgradeHook() noexcept -> UpgradeHook;

  static auto wifiClient() noexcept -> WiFiClient &;

  static void disconnect() noexcept;
  static auto isConnected() noexcept -> bool;

  auto httpPut(StringView token, StaticString path,
               StringView data) const noexcept
      -> Result<Response, int>;
  auto httpPost(StringView token, StringView path,
                StringView data) const noexcept
      -> Result<Response, int>;
  auto httpPost(StringView token, StaticString path,
                StringView data) const noexcept
      -> Result<Response, int>;
  auto httpPost(StaticString path, StringView data) const noexcept
      -> Result<Response, int>;

  auto httpRequest(HttpMethod method, const std::optional<StringView> &token,
                   StringView path,
                   const std::optional<StringView> &data) const noexcept
      -> Result<Response, int>;

  static auto rawStatusToString(const RawStatus &status) noexcept
      -> StaticString;
  auto rawStatus(int code) const noexcept -> RawStatus;

  static auto apiStatusToString(const NetworkStatus &status) noexcept
      -> StaticString;
  auto apiStatus(const RawStatus &raw) const noexcept
      -> std::optional<NetworkStatus>;

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
  NetworkStatus status;
  std::optional<std::string> payload;
  ~Response() noexcept;
  explicit Response(const NetworkStatus &status) noexcept;
  Response(const NetworkStatus &status, std::string payload) noexcept;
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
} // namespace iop
#endif