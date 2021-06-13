#include "api.hpp"
#include "core/cert_store.hpp"
#include "core/string/cow.hpp"
#include "generated/certificates.hpp"
#include "utils.hpp"
#include "core/lazy.hpp"
#include <string>

static iop::Lazy<iop::CertStore> certStore([]() { return iop::CertStore(generated::certList); });

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report CLIENT_BUFFER_OVERFLOWS

#ifndef IOP_API_DISABLED

#include "driver/client.hpp"

#ifndef IOP_DESKTOP
static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE

#ifndef IOP_DESKTOP
  // Makes sure the event handlers are never dropped and only set once
  static const auto onCallback = [](const WiFiEventStationModeGotIP &ev) {
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
    (void)ev;
  };
  static const auto onHandler = WiFi.onStationModeGotIP(onCallback);
  // Initialize the wifi configurations

  if (iop::Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));
#endif

  this->network().setup();

#ifdef IOP_SSL
  // client.setInsecure();
  // We should make sure our server supports Max Fragment Length Negotiation
  // if (client.probeMaxFragmentLength(uri, port, 512))
  //     client.setBufferSizes(512, 512);
#endif

#endif
}

auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Report iop_panic: "), event.msg);

  // This should fit everything, but as a critical endpoint
  // We truncate the message until it does (the metadata is never that big)
  constexpr uint16_t bytes = 2048;

  std::optional<std::string> truncatedMessage;
  std::optional<iop::FixedString<bytes>> maybeJson;

  while (true) {
    const auto make = [event, &truncatedMessage](JsonDocument &doc) {
      doc["file"] = event.file.toStdString();
      doc["line"] = event.line;
      // TODO: when ArduinoJson provides support for string_view we should stop this `.begin()` nonsense
      doc["func"] = event.func.begin();
      if (truncatedMessage.has_value()) {
        doc["msg"] = iop::unwrap_ref(truncatedMessage, IOP_CTX());
      } else {
        doc["msg"] = event.msg.begin();
      }
    };
    maybeJson = this->makeJson<bytes>(F("Api::reportPanic"), make);

    if (!maybeJson.has_value()) {
      if (truncatedMessage.has_value()) {
        auto &msg = iop::unwrap_mut(truncatedMessage, IOP_CTX());
        if (msg.length() == 0) break;
        msg.resize(msg.length() / 2);
      } else {
        truncatedMessage.emplace(event.msg.begin(), event.msg.length() / 2);
      }
      continue;
    }
    break;
  }

  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX());

  const auto token = authToken.asString();
  const auto maybeResp = this->network().httpPost(token.get(), F("/v1/panic"), json.get());

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
  auto maybeJson = this->makeJson<256>(F("Api::registerEvent"), make);
  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX());

  const auto token = authToken.asString();
  const auto maybeResp = this->network().httpPost(token.get(), F("/v1/event"), json.get());

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

  this->logger.debug(F("Authenticate IoP user: "), std::string(username));

  if (!username.length() || !password.length()) {
    this->logger.debug(F("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }

  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = std::move(username).begin();
    doc["password"] = std::move(password).begin();
  };
  auto maybeJson = this->makeJson<1024>(F("Api::authenticate"), make);
  if (!maybeJson.has_value())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = iop::unwrap(maybeJson, IOP_CTX());

  auto maybeResp = this->network().httpPost(F("/v1/user/login"), json.get());

#ifndef IOP_MOCK_MONITOR
  if (iop::is_err(maybeResp)) {
    const auto code = std::to_string(iop::unwrap_err_ref(maybeResp, IOP_CTX()));
    this->logger.error(F("Unexpected response at Api::authenticate: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto resp = std::move(iop::unwrap_ok_mut(maybeResp, IOP_CTX()));

  if (resp.status != iop::NetworkStatus::OK) {
    return resp.status;
  }

  if (!resp.payload.has_value()) {
    this->logger.error(F("Server answered OK, but payload is missing"));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto &payload = iop::unwrap_ref(resp.payload, IOP_CTX());
  auto result = AuthToken::fromString(payload);
  if (iop::is_err(result)) {
    const auto lengthStr = std::to_string(payload.length());

    const iop::StaticString parseErr(F("Unprintable payload, this isn't supported: "));
    const auto &ref = iop::unwrap_err_ref(result, IOP_CTX());
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      this->logger.error(F("Auth token is too big: size = "), lengthStr);
      return iop::NetworkStatus::BROKEN_SERVER;
    case iop::ParseError::NON_PRINTABLE:
      this->logger.error(parseErr,
                         iop::to_view(iop::scapeNonPrintable(payload)));
      return iop::NetworkStatus::BROKEN_SERVER;
    }
    this->logger.error(F("Unexpected ParseError: "),
                       std::to_string(static_cast<uint8_t>(ref)));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  return iop::unwrap_ok_mut(result, IOP_CTX());
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      std::string_view log) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = authToken.asString();
  this->logger.debug(F("Register log. Token: "), iop::to_view(token), F(". Log: "), log);
  auto maybeResp = this->network().httpPost(token.get(), F("/v1/log"), std::move(log));

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
const static std::string emptyString;
#else
#include "ESP8266httpUpdate.h"
#endif

auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Upgrading sketch"));

  #ifdef IOP_DESKTOP
  (void) token;
  #else
  const auto tok = token.asString();
  const auto path = F("/v1/update");

  #ifndef IOP_DESKTOP
  const auto uri = String(this->uri().get()) + path;
  #else
  const auto uri = std::string(this->uri().asCharPtr()) + iop::StaticString(path).asCharPtr();
  #endif

  auto &client = iop::Network::wifiClient();

  ESPhttpUpdate.setAuthorization(tok.get());
  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  ESPhttpUpdate.rebootOnUpdate(true);
  ESPhttpUpdate.setLedPin(LED_BUILTIN);
  const auto result = ESPhttpUpdate.updateFS(client, uri, emptyString);

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
  return AuthToken::empty();
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
    : logger(logLevel, F("API")), network_(std::move(uri), logLevel) {
  IOP_TRACE();
}

Api::Api(Api const &other) : logger(other.logger), network_(other.network_) {
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