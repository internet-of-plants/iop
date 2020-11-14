#ifndef IOP_SERVER_H_
#define IOP_SERVER_H_

#include <memory>
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <static_string.hpp>
#include <option.hpp>
#include <result.hpp>
#include <api.hpp>
#include <log.hpp>
#include <flash.hpp>

enum ServeError {
  REMOVE_WIFI_CONFIG
};

/// Server to safely acquire wifi and Internet of Plants credentials
///
/// It provides a access point with a captive portal.
/// The captive portal will provide a form with dynamic fields
/// Two fields (network ssid and psk) for the wifi credentials, if you are not authenticated
/// Two fields (email and password) for IoP credentials, if you don't have a authentication token stored
///
/// It opens itself at `serve`, but it should be manually closed `close` when wifi + IoP are authenticated
class CredentialsServer {
private:
  Log logger;
  std::shared_ptr<Api> api;
  std::shared_ptr<Flash> flash;
  Option<std::shared_ptr<ESP8266WebServer>> server;
  Option<std::unique_ptr<DNSServer>> dnsServer;
  unsigned long nextTryFlashWifiCredentials = 0;
  unsigned long nextTryHardcodedWifiCredentials = 0;
  unsigned long nextTryHardcodedIopCredentials = 0;
public:
  station_status_t connect(const StringView ssid, const StringView password) const;
  Result<AuthToken, Option<HttpCode>> authenticate(const StringView username, const StringView password) const;

  CredentialsServer(const StaticString host, const LogLevel logLevel):
    logger(logLevel, F("SERVER")),
    api(new Api(host, logLevel)),
    flash(new Flash(logLevel)) {}
  CredentialsServer(CredentialsServer& other) = delete;
  void operator=(CredentialsServer& other) = delete;

  CredentialsServer(CredentialsServer&& other):
    logger(other.logger.level(), other.logger.target()),
    api(new Api(other.api->host(), other.api->loggerLevel())),
    flash(new Flash(other.logger.level())),
    server(std::move(other.server)),
    dnsServer(std::move(other.dnsServer)),
    nextTryFlashWifiCredentials(0),
    nextTryHardcodedWifiCredentials(0),
    nextTryHardcodedIopCredentials(0) {}

  void operator=(CredentialsServer&& other) {
    this->api = std::move(other.api);
    this->logger = std::move(other.logger);
    this->flash = std::move(other.flash);
    this->server = std::move(other.server);
    this->dnsServer = std::move(other.dnsServer);
    this->nextTryFlashWifiCredentials = other.nextTryFlashWifiCredentials;
    this->nextTryHardcodedWifiCredentials = other.nextTryHardcodedWifiCredentials;
    this->nextTryHardcodedIopCredentials = other.nextTryHardcodedIopCredentials;
  }

  Result<Option<AuthToken>, ServeError> serve(const Option<struct WifiCredentials> & storedWifi, const Option<AuthToken> & authToken);
  void close();
  void start();
};

#include <utils.hpp>
#ifndef IOP_ONLINE
  #define IOP_SERVER_DISABLED
#endif
#ifndef IOP_SERVER
  #define IOP_SERVER_DISABLED
#endif

#endif
