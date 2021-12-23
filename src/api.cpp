#include "driver/client.hpp"
#include "driver/server.hpp"
#include "core/panic.hpp"
#include "generated/certificates.hpp"
#include "api.hpp"
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
  
  auto doc = std::make_unique<StaticJsonDocument<768>>(); // TODO: handle OOM here
  doc->clear();
  jsonObjectBuilder(*doc);

  if (doc->overflowed()) {
    this->logger.error(FLASH("Payload doesn't fit Json<768> at "), contextName);
    return std::nullopt;
  }

  auto &fixed = unused4KbSysStack.text();
  fixed.fill('\0');
  serializeJson(*doc, fixed.data(), fixed.max_size());
  
  this->logger.debug(FLASH("Json: "), iop::to_view(fixed));
  return std::ref(fixed);
}

#ifdef IOP_ONLINE
#ifndef IOP_DESKTOP
static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}
void wifiCredentialsCallback() noexcept {
  utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
}
#endif
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE

#ifndef IOP_DESKTOP
  iop::data.wifi.onStationModeGotIP(wifiCredentialsCallback);
  // Initialize the wifi configurations

  if (iop::Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));
#endif

#ifdef IOP_SSL
  static driver::CertStore certStore(generated::certList);
  this->network.setCertStore(certStore);
#endif

  this->network.setup();
#endif
}

auto Api::reportPanic(const AuthToken &authToken, const PanicData &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(FLASH("Report iop_panic: "), event.msg);

  auto msg = event.msg;
  std::optional<std::reference_wrapper<std::array<char, 768>>> maybeJson;

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    maybeJson = this->makeJson(FLASH("Api::reportPanic"), make);

    if (!maybeJson) {
      iop_assert(msg.length() / 2 != 0, FLASH("Message would be empty, function is broken"));
      msg = msg.substr(0, msg.length() / 2);
      continue;
    }
    break;
  }

  if (!maybeJson)
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = maybeJson->get();

  const auto token = iop::to_view(authToken);
  auto const status = this->network.httpPost(token, FLASH("/v1/panic"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(FLASH("Unexpected response at Api::reportPanic: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status;
  }
  iop_panic(FLASH("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::registerEvent(const AuthToken &authToken,
                        const Event &event) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(FLASH("Send event"));

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  };
  // 256 bytes is more than enough (we checked, it doesn't get to 200 bytes)
  auto maybeJson = this->makeJson(FLASH("Api::registerEvent"), make);
  if (!maybeJson)
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = maybeJson->get();

  const auto token = iop::to_view(authToken);
  auto const status = this->network.httpPost(token, FLASH("/v1/event"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(FLASH("Unexpected response at Api::registerEvent: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status;
  }
  iop_panic(FLASH("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}
auto Api::authenticate(std::string_view username,
                       std::string_view password) const noexcept
    -> std::variant<AuthToken, iop::NetworkStatus> {
  IOP_TRACE();

  this->logger.info(FLASH("Authenticate IoP user: "), username);

  if (!username.length() || !password.length()) {
    this->logger.warn(FLASH("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }

  const auto make = [username, password](JsonDocument &doc) {
    IOP_TRACE();
    doc["email"] = username;
    doc["password"] = password;
  };
  auto maybeJson = this->makeJson(FLASH("Api::authenticate"), make);

  if (!maybeJson)
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = maybeJson->get();
  
  auto const status = this->network.httpPost(FLASH("/v1/user/login"), iop::to_view(json));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(FLASH("Unexpected response at Api::authenticate: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    const auto &resp = *response;

    if (resp.status != iop::NetworkStatus::OK) {
      return resp.status;
    }

    if (!resp.payload) {
      this->logger.error(FLASH("Server answered OK, but payload is missing"));
      return iop::NetworkStatus::BROKEN_SERVER;
    }
    const auto &payload = *resp.payload;

    if (!iop::isAllPrintable(payload)) {
      this->logger.error(FLASH("Unprintable payload, this isn't supported: "), iop::to_view(iop::scapeNonPrintable(payload)));
      return iop::NetworkStatus::BROKEN_SERVER;
    }
    if (payload.length() != 64) {
      this->logger.error(FLASH("Auth token does not occupy 64 bytes: size = "), std::to_string(payload.length()));
    }

    AuthToken token;
    memcpy(token.data(), payload.c_str(), 64);
    return token;
  }
  iop_panic(FLASH("Invalid variant"));
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      std::string_view log) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.info(FLASH("Register log. Token: "), token, FLASH(". Log: "), log);
  auto const status = this->network.httpPost(token, FLASH("/v1/log"), std::move(log));

#ifndef IOP_MOCK_MONITOR
  if (const auto *error = std::get_if<int>(&status)) {
    const auto code = std::to_string(*error);
    this->logger.error(FLASH("Unexpected response at Api::registerLog: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  } else if (const auto *response = std::get_if<iop::Response>(&status)) {
    return response->status;
  }
  iop_panic(FLASH("Invalid variant"));
#else
  return iop::NetworkStatus::OK;
#endif
}

#ifdef IOP_DESKTOP
#define LED_BUILTIN 0
//#include "driver/upgrade.hpp"
#else
#define HIGH 0x1
#include "ESP8266httpUpdate.h"
#endif

auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(FLASH("Upgrading sketch"));

  #ifdef IOP_DESKTOP
  (void) token;
  #else
  const iop::StaticString path = FLASH("/v1/update");
  const auto uri = String(this->network.uri().get()) + path.get();

  auto *client = iop::data.wifi.client;
  iop_assert(client, F("Wifi has been moved out, client is nullptr"));

  auto ESPhttpUpdate = std::make_unique<ESP8266HTTPUpdate>();
  iop_assert(ESPhttpUpdate, FLASH("Unable to allocate ESP8266HTTPUpdate"));
  ESPhttpUpdate->setAuthorization(std::string(iop::to_view(token)).c_str());
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
  #ifndef IOP_DESKTOP
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    this->logger.error(FLASH("Update failed: "),
                       iop::to_view(ESPhttpUpdate->getLastErrorString()));
  #endif
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  #ifndef IOP_DESKTOP
  // TODO(pc): properly handle ESPhttpUpdate.getLastError()
  this->logger.error(FLASH("Update failed (UNKNOWN): "),
                     iop::to_view(ESPhttpUpdate->getLastErrorString()));
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
auto Api::authenticate(std::string_view username,
                       std::string_view password) const noexcept
    -> std::variant<AuthToken, iop::NetworkStatus> {
  (void)*this;
  (void)std::move(username);
  (void)std::move(password);
  IOP_TRACE();
  return (AuthToken){0};
}
auto Api::registerLog(const AuthToken &authToken,
                      std::string_view log) const noexcept
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
    : network(std::move(uri), logLevel), logger(logLevel, FLASH("API")) {
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