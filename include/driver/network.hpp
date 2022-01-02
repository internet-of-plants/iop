#ifndef IOP_DRIVER_NETWORK_HPP
#define IOP_DRIVER_NETWORK_HPP

#include "driver/client.hpp"
#include "driver/log.hpp"

namespace iop {
struct Data {
  driver::HTTPClient http;
  driver::Wifi wifi;
  std::array<char, 32> ssid;
  std::array<char, 32> md5;
  std::array<char, 17> mac;
  Data() noexcept : ssid{0}, md5{0}, mac{0} {}
};

extern Data data;

/// Higher level error reporting. Lower level is handled by core
enum class NetworkStatus {
  CONNECTION_ISSUES,
  BROKEN_SERVER,
  CLIENT_BUFFER_OVERFLOW, // BROKEN_CLIENT

  OK,
  FORBIDDEN,
};

/// Hook to schedule the next firmware binary update (_must not_ actually update, but only use it to schedule an update for the next loop run)
///
/// It's called by `iop::Network` whenever the server sets the `LAST_VERSION` HTTP header, to a value that isn't the MD5 of the current firmware binary.
class UpgradeHook {
public:
  using UpgradeScheduler = void (*) ();
  UpgradeScheduler schedule;

  constexpr explicit UpgradeHook(UpgradeScheduler scheduler) noexcept : schedule(scheduler) {}

  /// No-Op
  static void defaultHook() noexcept;
};


class Response {
public:
  int error;
  NetworkStatus status;
  std::optional<std::string> payload;

  explicit Response(int error) noexcept;
  explicit Response(const NetworkStatus &status) noexcept;
  Response(const NetworkStatus &status, std::string payload) noexcept;

  ~Response() noexcept = default;
  Response(Response &resp) noexcept = delete;
  Response(Response &&resp) noexcept = default;
  auto operator=(Response &resp) noexcept -> Response & = delete;
  auto operator=(Response &&resp) noexcept -> Response & = default;
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

/// General lower level HTTP(S) client, that is focused on our need.
/// Which are security, good error reporting, no UB possible and ergonomy, in order.
///
/// Higher level than the simple ESP8266 client network abstractions, but still focused on the HTTP(S) protocol
///
/// _Must call_ `iop::Network::setCertStore` otherwise TLS won't work. It _will_ panic.
///
/// `iop::Network::setUpgradeHook` to set the hook that is called to schedule updates.
class Network {
  Log logger;
  StaticString uri_;

  /// Sends a custom HTTP request that may be authenticated to the monitor server (primitive used by higher level methods)
  auto httpRequest(HttpMethod method, const std::optional<iop::StringView> &token, StaticString path, const std::optional<iop::StringView> &data) const noexcept -> Response;
public:
  Network(StaticString uri, const LogLevel &logLevel) noexcept;

  auto setup() const noexcept -> void;
  auto uri() const noexcept -> StaticString { return this->uri_; };

  /// Sets the CertStore that will handle TLS requests (find the appropriate cert for the connection)
  ///
  /// Ensure to sync NTP before using TLS (or it will report an invalid date for the certificates)
  static void setCertStore(::driver::CertStore &store) noexcept;

  /// Sets new firmware update hook for this. Very useful to support upgrades
  /// reported by the network (LAST_VERSION header different than current
  /// sketch hash) Default is a noop
  static void setUpgradeHook(UpgradeHook scheduler) noexcept;
  /// Removes current hook, replaces for default one (noop)
  static auto takeUpgradeHook() noexcept -> UpgradeHook;

  /// Disconnects from WiFi
  static void disconnect() noexcept;
  /// Checks if WiFi connection is available (doesn't ensure WiFi actually has internet connection)
  static auto isConnected() noexcept -> bool;

  /// Sends an HTTP post that is authenticated to the monitor server.
  auto httpPost(iop::StringView token, StaticString path, iop::StringView data) const noexcept -> Response;

  /// Sends an HTTP post that is not authenticated to the monitor server (used for authentication).
  auto httpPost(StaticString path, iop::StringView data) const noexcept -> Response;

  /// Allows properly logging network status
  static auto apiStatusToString(const NetworkStatus &status) noexcept -> StaticString;
  /// Extracts a network status from the raw response
  auto apiStatus(const driver::RawStatus &raw) const noexcept -> std::optional<NetworkStatus>;

  ~Network() noexcept = default;
  Network(Network const &other) = default;
  Network(Network &&other) = delete;
  auto operator=(Network const &other) -> Network & = default;
  auto operator=(Network &&other) -> Network & = delete;
};

} // namespace iop
#endif