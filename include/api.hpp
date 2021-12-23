#ifndef IOP_API_HPP
#define IOP_API_HPP

#include "core/network.hpp"
#include "utils.hpp"
#include <ArduinoJson.h>
#undef LED_BUILTIN
#undef OUTPUT

class PanicData;

/// High level client, abstracts the monitor server's API in a safe and ergonomic way
///
/// In production this requires TLS
///
/// The device must be connected to a WiFi network for it to properly work.
///
/// If some method returns `NetworkStatus::CLIENT_BUFFER_OVERFLOW` the method is broken.
/// It exists to allow for monitoring methods to keep working during critical failures.
class Api {
private:
  iop::Network network;
  iop::Log logger;

public:
  Api(iop::StaticString uri, iop::LogLevel logLevel) noexcept;

  /// Initializes the networking internals, including TLS configuration
  void setup() const noexcept;

  /// Sends a panic message to the monitor server.
  ///
  /// Truncates the message as needed to avoid OOM.
  ///
  /// Return values:
  ///
  /// OK: success
  /// FORBIDDEN: auth token is invalid
  /// CONNECTION_ISSUES: problems with connection, retry later?
  /// CLIENT_BUFFER_OVERFLOW: unreachable, as the route truncates the message until it fits the buffer
  /// BROKEN_SERVER: must wait until the server is fixed
  auto reportPanic(const AuthToken &authToken, const PanicData &event) const noexcept -> iop::NetworkStatus;

  /// Sends a monitoring event to the server, also sends device metadata.
  ///
  /// Return values:
  ///
  /// OK: success
  /// FORBIDDEN: auth token is invalid
  /// CONNECTION_ISSUES: problems with the connection, retry later?
  /// CLIENT_BUFFER_OVERFLOW: critical, means the method is broken and a buffer overflows happened
  /// BROKEN_SERVER: must wait until the server is fixed
  auto registerEvent(const AuthToken &token, const Event &event) const noexcept -> iop::NetworkStatus;

  /// Tries to authenticate to the server, getting an AuthToken on success.
  ///
  /// Return values:
  ///
  /// OK: unreachable, as success returns an AuthToken
  /// FORBIDDEN: unreachable, as this route doesn't expect an AuthToken
  /// CONNECTION_ISSUES: problems with connection, retry later?
  /// CLIENT_BUFFER_OVERFLOW: critical, means the method is broken and a buffer overflows happened
  /// BROKEN_SERVER: must wait until server is fixed
  auto authenticate(std::string_view username, std::string_view password) const noexcept -> std::variant<AuthToken, iop::NetworkStatus>;

  /// Reports log message to server.
  ///
  /// Return values:
  ///
  /// OK: success
  /// FORBIDDEN: auth token is invalid
  /// CONNECTION_ISSUES: problems with connection, retry later?
  /// CLIENT_BUFFER_OVERFLOW: critical, means the method is broken and a buffer overflows happened
  /// BROKEN_SERVER: must wait until server is fixed
  auto registerLog(const AuthToken &authToken, std::string_view log) const noexcept -> iop::NetworkStatus;

  /// Tries to update the firmware. Returns OK if no updates are available.
  ///
  /// Doesn't return on success, as the device is restarted.
  ///
  /// OK: means no update is available
  /// FORBIDDEN: auth token is invalid
  /// CONNECTION_ISSUES: problems with connection, retry later?
  /// CLIENT_BUFFER_OVERFLOW: unreachable as this route doesn't use the payload buffer
  /// BROKEN_SERVER: must wait until server is fixed
  auto upgrade(const AuthToken &token) const noexcept -> iop::NetworkStatus;

private:
  using JsonCallback = std::function<void(JsonDocument &)>;

  /// Abstracs safe json serialization. Returns None on overflow.
  ///
  /// Generally it is a critical error and probably will break the system,
  /// But some methods truncate the messages and try again, if they are critical.
  ///
  /// Gets a name for logging purposes. And a callback that insert data into the JSON serializer abstraction.
  auto makeJson(const iop::StaticString contextName, const JsonCallback &jsonObjectBuilder) const noexcept -> std::optional<std::reference_wrapper<std::array<char, 768>>>;
public:
  ~Api() noexcept;
  Api(Api const &other);
  Api(Api &&other) = delete;
  auto operator=(Api const &other) -> Api &;
  auto operator=(Api &&other) -> Api & = delete;
};

/// Represents the data passed to the panic hook
struct PanicData {
  std::string_view msg;
  iop::StaticString file;
  uint32_t line;
  iop::StaticString func;
};

#ifndef IOP_MONITOR
#define IOP_API_DISABLED
#endif

#endif
