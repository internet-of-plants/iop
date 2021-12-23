#ifndef IOP_SERVER_HPP
#define IOP_SERVER_HPP

#include "driver/log.hpp"
#include "utils.hpp"
#include <optional>

class Api;

/// Server to safely collect wifi and Internet of Plants credentials from a HTML form.
///
/// It provides an access point with a captive portal.
/// The captive portal will provide a form with two fields (network ssid and psk)
/// for the wifi credentials and two fields (email and password) for monitor server
/// credentials.
///
/// Internet of Plants credentials don't persist anywhere, they are simply used
/// to request an authentication token for this device from the monitor server.
///
/// It opens itself at the first call to `serve`, but it should be manually closed
/// with `close` when wifi + monitor server credentials are retrieved
///
/// It connects to WiFi directly. So you need a callback to detect when that happens
/// to be able to store the credentials to flash
class CredentialsServer {
private:
  iop::Log logger;

  bool isServerOpen = false;

  /// Internal method to initialize the credential server, if not running
  void start() noexcept;

public:
  explicit CredentialsServer(const iop::LogLevel &logLevel) noexcept: logger(logLevel, FLASH("SERVER")) {}

  /// Setups everything the Captive Portal needs
  void setup() const noexcept;

  /// Serves the captive portal and handles each user connected to each,
  /// authenticating to the wifi and the monitor server when possible
  auto serve(const Api &api) noexcept -> std::optional<AuthToken>;

  /// Closes the Captive Portal if it's still open
  void close() noexcept;
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_SERVER_DISABLED
#endif
#ifndef IOP_SERVER
#define IOP_SERVER_DISABLED
#endif

#endif
