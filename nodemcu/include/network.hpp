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
    Network(const StaticString host, const LogLevel logLevel) noexcept:
      host_(host),
      logger(logLevel, F("NETWORK")) {}
    Network(Network& other) = delete;
    Network(Network&& other) = delete;
    Network& operator=(Network& other) = delete;
    Network& operator=(Network&& other) = delete;
    void setup() const noexcept;
    bool isConnected() const noexcept;
    Option<Response> httpPut(const StringView token, const StaticString path, const StringView data) const noexcept;
    Option<Response> httpPost(const StringView token, const StaticString path, const StringView data) const noexcept;
    Option<Response> httpPost(const StaticString path, const StringView data) const noexcept;
    Option<Response> httpRequest(const HttpMethod method, const Option<StringView> token, const StaticString path, const Option<StringView> data) const noexcept;
    StaticString wifiCodeToString(const wl_status_t val) const noexcept;
    String macAddress() const noexcept;
    StaticString host() const noexcept { return this->host_; };
    void disconnect() const noexcept;
};

#include <utils.hpp>
#ifndef IOP_ONLINE
  #define IOP_NETWORK_DISABLED
#endif

#endif