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

class CredentialsServer {
private:
  uint64_t secretKey;
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
    secretKey(utils::random()),
    logger(logLevel, F("SERVER")),
    api(new Api(host, logLevel)),
    flash(new Flash(logLevel)) {}
  CredentialsServer(CredentialsServer& other) = delete;
  void operator=(CredentialsServer& other) = delete;

  CredentialsServer(CredentialsServer&& other):
    secretKey(utils::random()),
    logger(other.logger.level(), other.logger.target()),
    api(new Api(other.api->host(), other.api->loggerLevel())),
    flash(new Flash(other.logger.level())),
    server(std::move(other.server)),
    dnsServer(std::move(other.dnsServer)),
    nextTryFlashWifiCredentials(0),
    nextTryHardcodedWifiCredentials(0),
    nextTryHardcodedIopCredentials(0) {}

  void operator=(CredentialsServer&& other) {
    this->secretKey = utils::random();
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
