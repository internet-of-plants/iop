#include "api.hpp"
#include "core/cert_store.hpp"
#include "generated/certificates.hpp"
#include "utils.hpp"
#include <string>
#include "loop.hpp"
#include "ArduinoJson.h"

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report CLIENT_BUFFER_OVERFLOWS

#ifndef IOP_API_DISABLED

#include "driver/client.hpp"
#include "driver/server.hpp"

auto Api::makeJson(const iop::StaticString name, const JsonCallback &func) const noexcept
    -> std::optional<std::reference_wrapper<std::array<char, 1024>>> {
  IOP_TRACE();
  iop::logMemory(this->logger);

  auto &doc = unused4KbSysStack.json();
  doc.clear();
  func(doc);

  if (doc.overflowed()) {
    this->logger.error(F("Payload doesn't fit Json<1024> at "), name);
    return std::optional<std::reference_wrapper<std::array<char, 1024>>>();
  }

  auto &fixed = unused4KbSysStack.text();
  fixed.fill('\0');
  serializeJson(doc, fixed.data(), fixed.max_size());
  this->logger.debug(F("Json: "), iop::to_view(fixed));
  return std::make_optional(std::ref(fixed));
}

#ifdef IOP_ONLINE
#ifndef IOP_DESKTOP
static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}
void wifiCredentialsCallback(const WiFiEventStationModeGotIP &ev) noexcept {
  utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
  (void)ev;
}
#endif
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE

#ifndef IOP_DESKTOP
  static const auto onHandler = WiFi.onStationModeGotIP(wifiCredentialsCallback);
  // Initialize the wifi configurations

  if (iop::Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));
#endif

#ifdef IOP_SSL
  static iop::CertStore certStore(generated::certList);
  this->network().setCertStore(certStore);
#endif

  this->network().setup();

#endif
}

auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Report iop_panic: "), event.msg);

  auto msg = event.msg;
  std::optional<std::reference_wrapper<std::array<char, 1024>>> maybeJson;

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    maybeJson = this->makeJson(F("Api::reportPanic"), make);

    if (!maybeJson.has_value()) {
      iop_assert(msg.length() / 2 != 0, F("Message would be empty, function is broken"));
      msg = msg.substr(0, msg.length() / 2);
      continue;
    }
    break;
  }

  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX()).get();

  const auto token = iop::to_view(authToken);
  auto const & maybeResp = this->network().httpPost(token, F("/v1/panic"), json.data());

#ifndef IOP_MOCK_MONITOR
  if (iop::is_err(maybeResp)) {
    const auto code = std::to_string(iop::unwrap_err_ref(maybeResp, IOP_CTX()));
    this->logger.error(F("Unexpected response at Api::reportPanic: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return iop::unwrap_ok_ref(maybeResp, IOP_CTX()).status;
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::registerEvent(const AuthToken &authToken,
                        const Event &event) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Send event"));

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.storage.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.storage.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.storage.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.storage.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.storage.soilResistivityRaw;
  };
  // 256 bytes is more than enough (we checked, it doesn't get to 200 bytes)
  auto maybeJson = this->makeJson(F("Api::registerEvent"), make);
  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX()).get();

  const auto token = iop::to_view(authToken);
  auto const & maybeResp = this->network().httpPost(token, F("/v1/event"), json.data());

#ifndef IOP_MOCK_MONITOR
  if (iop::is_err(maybeResp)) {
    const auto code = std::to_string(iop::unwrap_err_ref(maybeResp, IOP_CTX()));
    this->logger.error(F("Unexpected response at Api::registerEvent: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return iop::unwrap_ok_ref(maybeResp, IOP_CTX()).status;
#else
  return iop::NetworkStatus::OK;
#endif
}
auto Api::authenticate(std::string_view username,
                       std::string_view password) const noexcept
    -> std::variant<AuthToken, iop::NetworkStatus> {
  IOP_TRACE();

  this->logger.debug(F("Authenticate IoP user: "), username);

  if (!username.length() || !password.length()) {
    this->logger.debug(F("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }

  const auto make = [username, password](JsonDocument &doc) {
    IOP_TRACE();
    doc["email"] = username;
    doc["password"] = password;
  };
  auto maybeJson = this->makeJson(F("Api::authenticate"), make);

  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX()).get();
  
  auto const & maybeResp = this->network().httpPost(F("/v1/user/login"), json.data());

#ifndef IOP_MOCK_MONITOR
  if (iop::is_err(maybeResp)) {
    const auto code = std::to_string(iop::unwrap_err_ref(maybeResp, IOP_CTX()));
    this->logger.error(F("Unexpected response at Api::authenticate: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto & resp = iop::unwrap_ok_ref(maybeResp, IOP_CTX());

  if (resp.status != iop::NetworkStatus::OK) {
    return resp.status;
  }

  if (!resp.payload.has_value()) {
    this->logger.error(F("Server answered OK, but payload is missing"));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto & payload = iop::unwrap_ref(resp.payload, IOP_CTX());
  if (!iop::isAllPrintable(payload)) {
    this->logger.error(F("Unprintable payload, this isn't supported: "), iop::to_view(iop::scapeNonPrintable(payload)));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  if (payload.length() != 64) {
    this->logger.error(F("Auth token does not occupy 64 bytes: size = "), std::to_string(payload.length()));
  }

  memcpy(unused4KbSysStack.token().data(), payload.c_str(), 64);
  return unused4KbSysStack.token();
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      std::string_view log) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.debug(F("Register log. Token: "), token, F(". Log: "), log);
  auto const & maybeResp = this->network().httpPost(token, F("/v1/log"), std::move(log));

#ifndef IOP_MOCK_MONITOR
  if (iop::is_err(maybeResp)) {
    const auto code = std::to_string(iop::unwrap_err_ref(maybeResp, IOP_CTX()));
    this->logger.error(F("Unexpected response at Api::registerLog: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  return iop::unwrap_ok_ref(maybeResp, IOP_CTX()).status;
#else
  return iop::NetworkStatus::OK;
#endif
}

#ifdef IOP_DESKTOP
#define LED_BUILTIN 0
//#include "driver/upgrade.hpp"
#endif

auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Upgrading sketch"));

  #ifdef IOP_DESKTOP
  (void) token;
  #else
  const iop::StaticString path = F("/v1/update");

  #ifndef IOP_DESKTOP
  const auto uri = String(this->uri().get()) + path.get();
  #else
  const auto uri = std::string(this->uri().asCharPtr()) + path.asCharPtr();
  #endif

  auto &client = iop::Network::wifiClient();

  auto &ESPhttpUpdate = unused4KbSysStack.updater();
  ESPhttpUpdate.setAuthorization(token.data());
  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  ESPhttpUpdate.rebootOnUpdate(true);
  //ESPhttpUpdate.setLedPin(LED_BUILTIN);
  const auto result = ESPhttpUpdate.updateFS(client, uri, "");

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
    this->logger.error(F("Update failed: "),
                       iop::to_view(ESPhttpUpdate.getLastErrorString()));
  #endif
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  #ifndef IOP_DESKTOP
  // TODO(pc): properly handle ESPhttpUpdate.getLastError()
  this->logger.error(F("Update failed (UNKNOWN): "),
                     iop::to_view(ESPhttpUpdate.getLastErrorString()));
  #endif
  #endif
  return iop::NetworkStatus::BROKEN_SERVER;
}
#else
auto Api::loggerLevel() const noexcept -> iop::LogLevel {
  IOP_TRACE();
  return this->logger.level();
}
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
    : network_(std::move(uri), logLevel), logger(logLevel, F("API")) {
  IOP_TRACE();
}

Api::Api(Api const &other) : network_(other.network_), logger(other.logger) {
  IOP_TRACE();
}

auto Api::operator=(Api const &other) -> Api & {
  IOP_TRACE();
  if (this == &other)
    return *this;
  this->logger = other.logger;
  this->network_ = other.network_;
  return *this;
}

auto Api::uri() const noexcept -> iop::StaticString {
  return this->network().uri();
};

auto Api::network() const noexcept -> const iop::Network & {
  IOP_TRACE();
  return this->network_;
}