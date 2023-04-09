#ifndef IOP_SERVER_HPP
#define IOP_SERVER_HPP

#include "iop-hal/log.hpp"
#include "iop-hal/server.hpp"
#include "iop/utils.hpp"
#include <optional>

namespace iop {
struct DynamicIopCredential {
  std::string organization;
  std::string login;
  std::string password;

  DynamicIopCredential(std::string organization, std::string login, std::string password) noexcept: organization(organization), login(login), password(password) {}
};

struct DynamicWifiCredential {
  std::string login;
  std::string password;

  DynamicWifiCredential(std::string login, std::string password) noexcept: login(login), password(password) {}
};

struct StaticCredential {
  iop::StaticString login;
  iop::StaticString password;

  StaticCredential(iop::StaticString login, iop::StaticString password) noexcept: login(login), password(password) {}
};

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
/// to be able to store the credentials to storage
class CredentialsServer {
private:
  std::unique_ptr<StaticCredential> credentialsAccessPoint;
  std::unique_ptr<DynamicIopCredential> credentialsIop;
  std::unique_ptr<DynamicWifiCredential> credentialsWifi;

  Log logger;

  iop_hal::HttpServer server;
  iop_hal::CaptivePortal dnsServer;
  bool isServerOpen = false;


  /// Internal method to initialize the credential server, if not running
  auto start() noexcept -> void;

public:
  explicit CredentialsServer() noexcept;

  /// Setups everything the Captive Portal needs
  auto setup() noexcept -> void;

  /// Configures Access Point credentials, must be called before `serve`
  auto setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void;

  /// Serves the captive portal and handles each user connected to each,
  /// authenticating to the wifi and returning the monitor server credentials when available
  auto serve() noexcept -> std::unique_ptr<DynamicIopCredential>;

  /// Closes the Captive Portal if it's still open
  auto close() noexcept -> bool;
};
}
#endif
