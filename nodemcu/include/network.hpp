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

class Network {
  StringView host_;
  Log logger;

  public:
    Network(const StringView host, const LogLevel logLevel):
      host_(host),
      logger(logLevel, F("NETWORK")) {}
    Network(Network& other) = delete;
    void operator=(Network& other) = delete;
    void operator=(Network&& other) {
      this->host_ = std::move(other.host_);
      this->logger = std::move(other.logger);
    }
    Network(Network&& other):
      host_(other.host_),
      logger(other.logger.level(), other.logger.target()) {}
    void setup() const;
    bool isConnected() const;
    Option<Response> httpPut(const StringView token, const StringView path, const StringView data) const;
    Option<Response> httpPost(const StringView token, const StringView path, const StringView data) const;
    Option<Response> httpPost(const StringView path, const StringView data) const;
    Option<Response> httpRequest(const HttpMethod method, const Option<StringView> token, const StringView path, const Option<StringView> data) const;
    StaticString wifiCodeToString(const wl_status_t val) const;
    String macAddress() const;
    StringView host() const { return this->host_; };
    void disconnect() const;
};

#include <utils.hpp>
#ifndef IOP_ONLINE
  #define IOP_NETWORK_DISABLED
#endif

#endif