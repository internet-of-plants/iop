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

  esp_time nextTryFlashWifiCredentials = 0;
  esp_time nextTryHardcodedWifiCredentials = 0;
  bool isServerOpen = false;

public:
  ~CredentialsServer() = default;

  auto connect(StringView ssid, StringView password) const noexcept -> void;
  auto authenticate(StringView username, StringView password,
                    const Api &api) const noexcept -> Option<AuthToken>;

  explicit CredentialsServer(const LogLevel logLevel) noexcept
      : logger(logLevel, F("SERVER")) {}

  // Self-referential class, it must not be moved or copied. SELF_REF
  CredentialsServer(CredentialsServer const &other) = delete;
  CredentialsServer(CredentialsServer &&other) = delete;
  auto operator=(CredentialsServer const &other)
      -> CredentialsServer & = delete;
  auto operator=(CredentialsServer &&other) -> CredentialsServer & = delete;

  auto serve(const Option<WifiCredentials> &storedWifi, const Api &api) noexcept
      -> Option<AuthToken>;
  void close() noexcept;
  void start() noexcept;
  void setup() const noexcept;

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
