#ifndef IOP_NETWORK_H_
#define IOP_NETWORK_H_

#include <ESP8266WiFi.h>
#include <utils.hpp>
#include <log.hpp>

typedef struct response {
  uint16_t code;
  String payload;
} Response;

enum HttpMethod {
  GET,
  DELETE,
  POST,
  PUT
};

class Network {
  String host_;
  Log logger;

  public:
    Network(const String host, const LogLevel logLevel):
      host_(host),
      logger(logLevel, "NETWORK") {}
    Network(Network& other) = delete;
    void operator=(Network& other) = delete;
    void operator=(Network&& other) {
      this->host_ = std::move(other.host_);
      this->logger = std::move(other.logger);
    }
    Network(Network&& other):
      host_(other.host_),
      logger(other.logger.level(), other.logger.target()) {}
    void setup(std::function<void (const WiFiEventStationModeGotIP &)> onConnection) const;
    bool isConnected() const;
    Option<Response> httpPut(const String token, const String path, const String data) const;
    Option<Response> httpPost(const String token, const String path, const String data) const;
    Option<Response> httpPost(const String path, const String data) const;
    Option<Response> httpRequest(const HttpMethod method, const Option<String> token, const String path, const Option<String> data) const;
    String wifiCodeToString(const wl_status_t val) const;
    String macAddress() const { return WiFi.macAddress(); }
    void disconnect() const { WiFi.disconnect(); }
};

#endif