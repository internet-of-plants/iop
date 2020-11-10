#ifndef IOP_SERVER_H_
#define IOP_SERVER_H_

#include <memory>
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <option.hpp>
#include <api.hpp>
#include <log.hpp>
#include <flash.hpp>

enum ServeError {
  REMOVE_WIFI_CONFIG
};

class CredentialsServer {
  private:
    Api api;
    Log logger;
    Flash flash;
    // Could we use a std::unique_ptr or even drop the need for a pointer?
    Option<std::shared_ptr<ESP8266WebServer>> server;
    Option<std::unique_ptr<DNSServer>> dnsServer;
    unsigned long nextTryFlashWifiCredentials = 0;
    unsigned long nextTryHardcodedWifiCredentials = 0;
    unsigned long nextTryHardcodedIopCredentials = 0;

  public:
    CredentialsServer(const String host, const LogLevel logLevel):
      api(host, logLevel),
      logger(logLevel, "SERVER"),
      flash(logLevel) {}
    CredentialsServer(CredentialsServer& other) = delete;
    void operator=(CredentialsServer& other) = delete;
    
    CredentialsServer(CredentialsServer&& other):
      api(other.api.host(), other.api.loggerLevel()),
      logger(other.logger.level(), other.logger.target()),
      flash(other.logger.level()),
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

    Option<AuthToken> authenticateIop(const String username, const String password);
    station_status_t authenticateWifi(const String ssid, const String password);
    Result<Option<AuthToken>, ServeError> serve(Option<struct station_config> storedWifi, Option<AuthToken> authToken);
    void close();
    void start();
};

#include <utils.hpp>
#ifndef IOP_ONLINE
  #define IOP_SERVER_DISABLED
#endif
#ifndef IOP_SERVEr
  #define IOP_SERVER_DISABLED
#endif

#endif