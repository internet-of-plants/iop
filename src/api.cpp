#include "api.hpp"
#include "driver/client.hpp"
#include "driver/server.hpp"
#include "driver/panic.hpp"
#include "generated/certificates.hpp"
#include "utils.hpp"
#include "loop.hpp"
#include <string>

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report CLIENT_BUFFER_OVERFLOWS

#ifndef IOP_API_DISABLED

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally and pretend the request didn't fail
// If IOP_MONITOR is defined, then IOP_MOCK_MONITOR shouldn't be defined
#ifdef IOP_MONITOR
#undef IOP_MOCK_MONITOR
#endif

auto Api::makeJson(const iop::StaticString contextName, const JsonCallback &jsonObjectBuilder) const noexcept -> std::optional<std::reference_wrapper<std::array<char, 768>>> {
  IOP_TRACE();
  
  auto doc = std::unique_ptr<StaticJsonDocument<768>>(new (std::nothrow) StaticJsonDocument<768>());
  iop_assert(doc, IOP_STATIC_STRING("OOM")); // TODO: properly handle OOM
  doc->clear();
  jsonObjectBuilder(*doc);

  if (doc->overflowed()) {
    this->logger.error(IOP_STATIC_STRING("Payload doesn't fit Json<768> at "), contextName);
    return std::nullopt;
  }

  auto &fixed = unused4KbSysStack.text();
  fixed.fill('\0');
  serializeJson(*doc, fixed.data(), fixed.max_size());
  
  this->logger.debug(IOP_STATIC_STRING("Json: "), iop::to_view(fixed));
  return std::ref(fixed);
}

#ifdef IOP_ONLINE
static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}
void wifiCredentialsCallback() noexcept {
  utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
}
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE

  iop::data.wifi.onStationModeGotIP(wifiCredentialsCallback);
  // Initialize the wifi configurations

  if (iop::Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));

#ifdef IOP_SSL
  static driver::CertStore certStore(generated::certList);
  this->network.setCertStore(certStore);
#endif

  this->network.setup();
#endif
}

auto Api::reportPanic(const AuthToken &authToken, const PanicData &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STATIC_STRING("Report iop_panic: "), event.msg);

  auto msg = event.msg;
  std::optional<std::reference_wrapper<std::array<char, 768>>> maybeJson;

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    maybeJson = this->makeJson(IOP_STATIC_STRING("Api::reportPanic"), make);

    if (!maybeJson) {
      iop_assert(msg.length() / 2 != 0, IOP_STATIC_STRING("Message would be empty, function is broken"));
      msg = msg.substr(0, msg.length() / 2);
      continue;
    }
    break;
  }

  if (!maybeJson)
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = maybeJson->get();

  const auto token = iop::to_view(authToken);
  auto const response = this->network.httpPost(token, IOP_STATIC_STRING("/v1/panic"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (response.error) {
    const auto code = std::to_string(response.error);
    this->logger.error(IOP_STATIC_STRING("Unexpected response at Api::reportPanic: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else {
    return response.status;
  }
  iop_panic(IOP_STATIC_STRING("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::registerEvent(const AuthToken &authToken,
                        const Event &event) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STATIC_STRING("Send event"));

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  };
  // 256 bytes is more than enough (we checked, it doesn't get to 200 bytes)
  auto maybeJson = this->makeJson(IOP_STATIC_STRING("Api::registerEvent"), make);
  if (!maybeJson)
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = maybeJson->get();

  const auto token = iop::to_view(authToken);
  auto const response = this->network.httpPost(token, IOP_STATIC_STRING("/v1/event"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (response.error) {
    const auto code = std::to_string(response.error);
    this->logger.error(IOP_STATIC_STRING("Unexpected response at Api::registerEvent: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else  {
    return response.status;
  }
  iop_panic(IOP_STATIC_STRING("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}
auto Api::authenticate(iop::StringView username,
                       iop::StringView password) const noexcept
    -> std::pair<iop::NetworkStatus, std::optional<AuthToken>> {
  IOP_TRACE();

  this->logger.info(IOP_STATIC_STRING("Authenticate IoP user: "), username);

  if (!username.length() || !password.length()) {
    this->logger.warn(IOP_STATIC_STRING("Empty username or password, at Api::authenticate"));
    return std::make_pair(iop::NetworkStatus::FORBIDDEN, std::nullopt);
  }

  const auto make = [username, password](JsonDocument &doc) {
    IOP_TRACE();
    doc["email"] = username;
    doc["password"] = password;
  };
  auto maybeJson = this->makeJson(IOP_STATIC_STRING("Api::authenticate"), make);

  if (!maybeJson)
    return std::make_pair(iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW, std::nullopt);
  const auto &json = maybeJson->get();
  
  auto const response = this->network.httpPost(IOP_STATIC_STRING("/v1/user/login"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (response.error) {
    const auto code = std::to_string(response.error);
    this->logger.error(IOP_STATIC_STRING("Unexpected response at Api::authenticate: "), code);
    return std::make_pair(iop::NetworkStatus::BROKEN_SERVER, std::nullopt);
  } else {
    if (response.status != iop::NetworkStatus::OK) {
      return std::make_pair(response.status, std::nullopt);
    }

    if (!response.payload) {
      this->logger.error(IOP_STATIC_STRING("Server answered OK, but payload is missing"));
      return std::make_pair(iop::NetworkStatus::BROKEN_SERVER, std::nullopt);
    }
    const auto &payload = *response.payload;

    if (!iop::isAllPrintable(payload)) {
      this->logger.error(IOP_STATIC_STRING("Unprintable payload, this isn't supported: "), iop::to_view(iop::scapeNonPrintable(payload)));
      return std::make_pair(iop::NetworkStatus::BROKEN_SERVER, std::nullopt);
    }
    if (payload.length() != 64) {
      this->logger.error(IOP_STATIC_STRING("Auth token does not occupy 64 bytes: size = "), std::to_string(payload.length()));
    }

    AuthToken token;
    memcpy(token.data(), payload.c_str(), 64);
    return std::make_pair(iop::NetworkStatus::OK, token);
  }
  iop_panic(IOP_STATIC_STRING("Invalid variant"));
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      iop::StringView log) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.info(IOP_STATIC_STRING("Register log. Token: "), token, IOP_STATIC_STRING(". Log: "), log);
  auto const response = this->network.httpPost(token, IOP_STATIC_STRING("/v1/log"), std::move(log));

#ifndef IOP_MOCK_MONITOR
  if (response.error) {
    const auto code = std::to_string(response.error);
    this->logger.error(IOP_STATIC_STRING("Unexpected response at Api::registerLog: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else {
    return response.status;
  }
  iop_panic(IOP_STATIC_STRING("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

#ifdef IOP_ESP8266
#define HIGH 0x1
#include "ESP8266httpUpdate.h"
#endif

// TODO: move upgrade logic to driver
auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STATIC_STRING("Upgrading sketch"));

  #ifndef IOP_ESP8266
  (void) token;
  #else
  const iop::StaticString path = IOP_STATIC_STRING("/v1/update");
  const auto uri = String(this->network.uri().get()) + path.get();

  auto *client = iop::data.wifi.client;
  iop_assert(client, IOP_STATIC_STRING("Wifi has been moved out, client is nullptr"));

  auto ESPhttpUpdate = std::unique_ptr<ESP8266HTTPUpdate>(new (std::nothrow) ESP8266HTTPUpdate());
  iop_assert(ESPhttpUpdate, IOP_STATIC_STRING("Unable to allocate ESP8266HTTPUpdate"));
  ESPhttpUpdate->setAuthorization(iop::CowString(iop::to_view(token)).own().c_str());
  ESPhttpUpdate->closeConnectionsOnUpdate(true);
  ESPhttpUpdate->rebootOnUpdate(true);
  //ESPhttpUpdate->setLedPin(LED_BUILTIN);
  const auto result = ESPhttpUpdate->update(*client, uri, "");

#ifdef IOP_MOCK_MONITOR
  return iop::NetworkStatus::OK;
#endif

switch (result) {
  case HTTP_UPDATE_NO_UPDATES:
  case HTTP_UPDATE_OK:
    return iop::NetworkStatus::OK;

  case HTTP_UPDATE_FAILED:
  #ifdef IOP_ESP8266
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    this->logger.error(IOP_STATIC_STRING("Update failed: "),
                       iop::StringView(ESPhttpUpdate->getLastErrorString().c_str()));
  #endif
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  #ifdef IOP_ESP8266
  // TODO(pc): properly handle ESPhttpUpdate.getLastError()
  this->logger.error(IOP_STATIC_STRING("Update failed (UNKNOWN): "),
                     iop::StringView(ESPhttpUpdate->getLastErrorString().c_str()));
  #endif
  #endif
  return iop::NetworkStatus::BROKEN_SERVER;
}
#else
auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)token;
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}
auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)authToken;
  (void)event;
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}
auto Api::registerEvent(const AuthToken &token,
                        const Event &event) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)token;
  (void)event;
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}
auto Api::authenticate(iop::StringView username,
                       iop::StringView password) const noexcept
    -> std::pair<iop::NetworkStatus, std::optional<AuthToken>> {
  (void)*this;
  (void)std::move(username);
  (void)std::move(password);
  IOP_TRACE();
  return std::make_pair(iop::NetworkStatus::OK, (AuthToken){0});
}
auto Api::registerLog(const AuthToken &authToken,
                      iop::StringView log) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)authToken;
  (void)std::move(log);
  IOP_TRACE();
  return iop::NetworkStatus::OK;
}

auto Api::setup() const noexcept -> void {}
#endif

Api::~Api() noexcept { IOP_TRACE(); }

Api::Api(iop::StaticString uri, const iop::LogLevel logLevel) noexcept
    : network(std::move(uri), logLevel), logger(logLevel, IOP_STATIC_STRING("API")) {
  IOP_TRACE();
}

Api::Api(Api const &other) : network(other.network), logger(other.logger) {
  IOP_TRACE();
}

auto Api::operator=(Api const &other) -> Api & {
  IOP_TRACE();
  if (this == &other)
    return *this;
  this->logger = other.logger;
  this->network = other.network;
  return *this;
}