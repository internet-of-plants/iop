#ifndef IOP_SERVER_HPP
#define IOP_SERVER_HPP

#include "driver/server.hpp"
#include "driver/wifi.hpp"
#include "driver/thread.hpp"
#include "utils.hpp"

#include <optional>

class Api;

/// Server to safely acquire wifi and Internet of Plants credentials
///
/// It provides an access point with a captive portal.
/// The captive portal will provide a form: two fields (network ssid and psk)
/// for the wifi credentials na dtwo fields (email and password) for IoP
/// credentials.
///
/// Internet of Plants credentials don't persist anywhere, they are simply used
/// to generate an authentication token for this device.
///
/// It opens itself at `serve`, but it should be manually closed with `close`
/// when wifi + IoP are retrieved
///
/// Doesn't actually returns wifi credentials. It connects to WiFi directly. So
/// you need a callback to detect when that happens.
////
/// Tip: call Network::isConnected() to check if connected
class CredentialsServer {
private:
  iop::Log logger;

  bool isServerOpen = false;

  void start() noexcept;

public:
  explicit CredentialsServer(const iop::LogLevel &logLevel) noexcept
      : logger(logLevel, F("SERVER")) {
    IOP_TRACE();
  }

  void setup() const noexcept;
  auto serve(const Api &api) noexcept -> std::optional<AuthToken>;
  void close() noexcept;

  ~CredentialsServer() noexcept { IOP_TRACE(); }
  CredentialsServer(CredentialsServer const &other) = default;
  CredentialsServer(CredentialsServer &&other) = delete;
  auto operator=(CredentialsServer const &other)
      -> CredentialsServer & = default;
  auto operator=(CredentialsServer &&other) -> CredentialsServer & = delete;
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_SERVER_DISABLED
#endif
#ifndef IOP_SERVER
#define IOP_SERVER_DISABLED
#endif

#endif
