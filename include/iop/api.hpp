#ifndef IOP_API_HPP
#define IOP_API_HPP

#include "iop-hal/network.hpp"
#include "iop/utils.hpp"

#include <ArduinoJson.h>

namespace iop {
class PanicData;

// Arbitrary Event might not fit in regular JSON, so it's up to the user to calibrate that
// ESP32 might be needed for bigger events
#ifndef IOP_JSON_CAPACITY
#define IOP_JSON_CAPACITY 768
#endif

/// High level client, abstracts the monitor server's API in a safe and ergonomic way
///
/// In production this requires TLS
///
/// The device must be connected to a WiFi network for it to properly work.
///
/// If some method returns `NetworkStatus::BROKEN_CLIENT` the method is broken.
/// It exists to allow for monitoring methods to keep working during critical failures.
class Api {
private:
  iop::Network network;
  iop::Log logger;

public:
  static constexpr size_t JsonCapacity = IOP_JSON_CAPACITY;
  using Json = std::array<char, JsonCapacity>;

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
  /// UNAUTHORIZED: auth token is invalid
  /// IO_ERROR: problems with connection, retry later?
  /// BROKEN_CLIENT: unreachable, as the route truncates the message until it fits the buffer
  /// BROKEN_SERVER: must wait until the server is fixed
  auto reportPanic(const AuthToken &authToken, const PanicData &event) noexcept -> iop::NetworkStatus;

  /// Sends a monitoring event to the server, also sends device metadata.
  ///
  /// Return values:
  ///
  /// OK: success
  /// UNAUTHORIZED: auth token is invalid
  /// IO_ERROR: problems with the connection, retry later?
  /// BROKEN_SERVER: must wait until the server is fixed
  /// BROKEN_CLIENT: unreachable
  auto registerEvent(const AuthToken &token, const Api::Json &event) noexcept -> iop::NetworkStatus;

  /// Tries to authenticate to the server, getting an AuthToken on success.
  ///
  /// Return values:
  ///
  /// OK: unreachable, as success returns an AuthToken
  /// UNAUTHORIZED: unreachable, as this route doesn't expect an AuthToken
  /// IO_ERROR: problems with connection, retry later?
  /// BROKEN_CLIENT: critical, means the method is broken and a buffer overflows happened
  /// BROKEN_SERVER: must wait until server is fixed
  auto authenticate(std::string_view username, std::string_view password) noexcept -> std::variant<std::unique_ptr<AuthToken>, iop::NetworkStatus>;

  /// Reports log message to server.
  ///
  /// Return values:
  ///
  /// OK: success
  /// UNAUTHORIZED: auth token is invalid
  /// IO_ERROR: problems with connection, retry later?
  /// BROKEN_CLIENT: critical, means the method is broken and a buffer overflows happened
  /// BROKEN_SERVER: must wait until server is fixed
  auto registerLog(const AuthToken &authToken, std::string_view log) noexcept -> iop::NetworkStatus;

  /// Tries to update the firmware. Returns OK if no updates are available.
  ///
  /// Doesn't return on success, as the device is restarted.
  ///
  /// OK: means no update is available
  /// UNAUTHORIZED: auth token is invalid
  /// IO_ERROR: problems with connection, retry later?
  /// BROKEN_CLIENT: unreachable as this route doesn't use the payload buffer
  /// BROKEN_SERVER: must wait until server is fixed
  auto update(const AuthToken &token) noexcept -> iop_hal::UpdateStatus;

  using JsonCallback = std::function<void(JsonDocument &)>;

  /// Abstracs safe json serialization. Returns None on overflow.
  ///
  /// Generally it is a critical error and probably will break the system,
  /// But some methods truncate the messages and try again, if they are critical.
  ///
  /// Gets a name for logging purposes. And a callback that insert data into the JSON serializer abstraction.
  auto makeJson(iop::StaticString contextName, Api::JsonCallback jsonObjectBuilder) noexcept -> std::unique_ptr<Api::Json>;
};

/// Represents the data passed to the panic hook
class PanicData {
public:
  std::string_view msg;
  iop::StaticString file;
  uint32_t line;
  iop::StaticString func;

  PanicData(std::string_view msg_, iop::StaticString file_, uint32_t line_, iop::StaticString func_) noexcept:
    msg(msg_), file(file_), line(line_), func(func_) {}
};
}

#endif
