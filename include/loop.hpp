#ifndef IOP_LOOP
#define IOP_LOOP

#ifndef IOP_DESKTOP
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#define HIGH 0x1
#include "ESP8266httpUpdate.h"
#undef OUTPUT
#undef INPUT
#undef HIGH
#undef LOW
#undef RISING
#undef FALLING
#undef CHANGED
#undef LED_BUILTIN
#endif

#include "configuration.hpp"
#include "flash.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "api.hpp"
#include "utils.hpp"

#include <optional>

#include "driver/device.hpp"
#include "driver/thread.hpp"

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger;
  Flash flash_;
  Sensors sensors;

  iop::esp_time nextMeasurement;
  iop::esp_time nextYieldLog;
  iop::esp_time nextHandleConnectionLost;

public:
  Api const & api() const noexcept { return this->api_; }
  Flash const & flash() const noexcept { return this->flash_; }
  void setup() noexcept;
  void loop() noexcept;

private:
  void handleInterrupt(const InterruptEvent event, const std::optional<AuthToken> &maybeToken) const noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  auto operator=(EventLoop const &other) noexcept -> EventLoop & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->sensors = other.sensors;
    this->api_ = other.api_;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash_ = other.flash_;
    this->nextMeasurement = other.nextMeasurement;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  };
  auto operator=(EventLoop &&other) noexcept -> EventLoop & {
    IOP_TRACE();
    this->sensors = other.sensors;
    this->api_ = other.api_;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash_ = other.flash_;
    this->nextMeasurement = other.nextMeasurement;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  }
  ~EventLoop() noexcept { IOP_TRACE(); }
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger(logLevel_, F("LOOP")), flash_(logLevel_),
        sensors(config::soilResistivityPower, config::soilTemperature, config::airTempAndHumidity, config::dhtVersion),
        nextMeasurement(0), nextYieldLog(0), nextHandleConnectionLost(0) {
    IOP_TRACE();
  }
  EventLoop(EventLoop const &other) noexcept
      : credentialsServer(other.credentialsServer),
        api_(other.api_),
        logger(other.logger),
        flash_(other.flash_),
        sensors(other.sensors),
        nextMeasurement(other.nextMeasurement),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }
  EventLoop(EventLoop &&other) noexcept
      : credentialsServer(other.credentialsServer), api_(other.api_),
        logger(other.logger), flash_(other.flash_), sensors(other.sensors),
        nextMeasurement(other.nextMeasurement),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }
};

class Unused4KbSysStack {
  struct StackStruct {
    std::optional<EventLoop> loop;
    std::optional<StaticJsonDocument<1024>> json;
    std::array<char, 1024> text;
    //std::optional<std::pair<std::array<char, 128>, std::array<char, 128>>> iop;
    //std::optional<std::pair<std::array<char, 32>, std::array<char, 64>>> wifi;
    #ifndef IOP_DESKTOP
    std::optional<ESP8266HTTPUpdate> updater;
    std::optional<ESP8266WebServer> server;
    std::optional<DNSServer> dns;
    #ifdef IOP_SSL
    std::optional<BearSSL::WiFiClientSecure> client;
    #else
    std::optional<WiFiClient> client;
    #endif
    #else
    std::optional<WiFiClient> client;
    #endif
    std::optional<HTTPClient> http;
    std::array<char, 64> token;
    std::array<char, 64> psk;
    std::optional<std::variant<iop::Response, int>> response;
    std::array<char, 32> ssid;
    std::array<char, 32> md5;
    std::array<char, 17> mac;
  } *data;
  static_assert(sizeof(StackStruct) <= 4096);

public:
  Unused4KbSysStack() noexcept: data(reinterpret_cast<StackStruct *>(0x3FFFE000)) {
    memset((void*)0x3FFFE000, 0, 4096);
  }
  void reset() noexcept {
    memset((void*)0x3FFFE000, 0, 4096);
  }
  auto response() noexcept -> std::variant<iop::Response, int> & {
    if (!this->data->response.has_value())
      this->data->response = std::make_optional(0);
    return iop::unwrap_mut(this->data->response, IOP_CTX());
  }
  auto loop() noexcept -> EventLoop & {
    if (!this->data->loop.has_value())
      this->data->loop = std::make_optional(EventLoop(config::uri(), config::logLevel));
    return iop::unwrap_mut(this->data->loop, IOP_CTX());
  }
  #ifndef IOP_DESKTOP
  #ifdef IOP_SSL
  auto client() noexcept -> BearSSL::WiFiClientSecure & {
    if (!this->data->client.has_value())
      this->data->client = std::make_optional(BearSSL::WiFiClientSecure());
    return iop::unwrap_mut(this->data->client, IOP_CTX());
  }
  #else
  auto client() noexcept -> WiFiClient & {
    if (!this->data->client.has_value())
      this->data->client = std::make_optional(WiFiClient());
    return iop::unwrap_mut(this->data->client, IOP_CTX());
  }
  #endif
  auto dns() noexcept -> DNSServer & {
    if (!this->data->dns.has_value())
      this->data->dns = std::make_optional(DNSServer());
    return iop::unwrap_mut(this->data->dns, IOP_CTX());
  }
  auto server() noexcept -> std::optional<ESP8266WebServer> & {
    return this->data->server;
  }
  auto updater() noexcept -> ESP8266HTTPUpdate & {
    if (!this->data->updater.has_value())
      this->data->updater = std::make_optional(ESP8266HTTPUpdate());
    return iop::unwrap_mut(this->data->updater, IOP_CTX());
  }
  #else
  auto client() noexcept -> WiFiClient & {
    if (!this->data->client.has_value())
      this->data->client = std::make_optional(WiFiClient());
    return iop::unwrap_mut(this->data->client, IOP_CTX());
  }
  #endif
  auto  http() noexcept -> HTTPClient & {
    if (!this->data->http.has_value())
      this->data->http = std::make_optional(HTTPClient());
    return iop::unwrap_mut(this->data->http, IOP_CTX());
  }
  auto mac() noexcept -> std::array<char, 17> & {
    return this->data->mac;
  }
  auto md5() noexcept -> std::array<char, 32> & {
    return this->data->md5;
  }
  auto psk() noexcept -> std::array<char, 64> & {
    return this->data->psk;
  }
  auto ssid() noexcept -> std::array<char, 32> & {
    return this->data->ssid;
  }
  auto token() noexcept -> std::array<char, 64> & {
    return this->data->token;
  }
  auto json() noexcept -> StaticJsonDocument<1024> & {
    if (!this->data->json.has_value())
      this->data->json = std::make_optional(StaticJsonDocument<1024>());
    return iop::unwrap_mut(this->data->json, IOP_CTX());
  }
  auto text() noexcept -> std::array<char, 1024> & {
    return this->data->text;
  }
  // ...
};
extern Unused4KbSysStack unused4KbSysStack;

#endif