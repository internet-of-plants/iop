#ifndef IOP_NETWORK_H_
#define IOP_NETWORK_H_

#include <ESP8266WiFi.h>
#include <log.hpp>
#include <option.hpp>
#include <static_string.hpp>

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

/// Try to make the ESP8266 network API more pallatable for our needs
class Network {
  StaticString host_;
  Log logger;

  public:
    Network(const StaticString host, const LogLevel logLevel):
      host_(host),
      logger(logLevel, F("NETWORK")) {}
    Network(Network& other) = delete;
    Network(Network&& other) = delete;
    void operator=(Network& other) = delete;
    void operator=(Network&& other) = delete;
    void setup() const;
    bool isConnected() const;
    Option<Response> httpPut(const StringView token, const StaticString path, const StringView data) const;
    Option<Response> httpPost(const StringView token, const StaticString path, const StringView data) const;
    Option<Response> httpPost(const StaticString path, const StringView data) const;
    Option<Response> httpRequest(const HttpMethod method, const Option<StringView> token, const StaticString path, const Option<StringView> data) const;
    StaticString wifiCodeToString(const wl_status_t val) const;
    String macAddress() const;
    StaticString host() const { return this->host_; };
    void disconnect() const;
};

#include <utils.hpp>
#ifndef IOP_ONLINE
  #define IOP_NETWORK_DISABLED
#endif

#endif