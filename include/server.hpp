#ifndef IOP_SERVER_HPP
#define IOP_SERVER_HPP

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <memory>

#include <api.hpp>
#include <flash.hpp>
#include <log.hpp>
#include <option.hpp>
#include <result.hpp>
#include <static_string.hpp>

enum ServeError { INVALID_WIFI_CONFIG };

/// Server to safely acquire wifi and Internet of Plants credentials
///
/// It provides a access point with a captive portal.
/// The captive portal will provide a form with dynamic fields
/// Two fields (network ssid and psk) for the wifi credentials, if you are not
/// authenticated Two fields (email and password) for IoP credentials, if you
/// don't have a authentication token stored
///
/// It opens itself at `serve`, but it should be manually closed `close` when
/// wifi + IoP are authenticated
class CredentialsServer {
private:
  Log logger;
  std::shared_ptr<Api> api;
  std::shared_ptr<Flash> flash;
  Option<std::shared_ptr<ESP8266WebServer>> server;
  Option<std::unique_ptr<DNSServer>> dnsServer;
  unsigned long nextTryFlashWifiCredentials = 0; // NOLINT google-runtime-int
  // NOLINTNEXTLINE google-runtime-int
  unsigned long nextTryHardcodedWifiCredentials = 0;
  unsigned long nextTryHardcodedIopCredentials = 0; // NOLINT google-runtime-int

  using Server = std::shared_ptr<ESP8266WebServer>;
  void makeRouter(const Server &s) const noexcept;

public:
  ~CredentialsServer() = default;

  auto connect(StringView ssid, StringView password) const noexcept
      -> station_status_t;
  auto authenticate(StringView username, StringView password) const noexcept
      -> Result<AuthToken, ApiStatus>;

  CredentialsServer(const StaticString &host, const LogLevel logLevel) noexcept
      : logger(logLevel, F("SERVER")),
        api(try_make_unique<Api>(host, logLevel)),
        flash(try_make_unique<Flash>(Flash(logLevel))) {
    if (!api || !flash)
      panic_(F("Unnable to allocate api or flash"));
  }
  CredentialsServer(CredentialsServer const &other) = delete;
  CredentialsServer(CredentialsServer &&other) = delete;
  auto operator=(CredentialsServer &other) -> CredentialsServer & = delete;
  auto operator=(CredentialsServer &&other) -> CredentialsServer & = delete;

  auto serve(const Option<WifiCredentials> &storedWifi,
             const Option<AuthToken> &authToken) noexcept
      -> Result<Option<AuthToken>, ServeError>;
  void close() noexcept;
  void start() noexcept;

  auto statusToString(station_status_t status) const noexcept
      -> Option<StaticString>;
};

#include "utils.hpp"
#ifndef IOP_ONLINE
#define IOP_SERVER_DISABLED
#endif
#ifndef IOP_SERVER
#define IOP_SERVER_DISABLED
#endif

#endif
