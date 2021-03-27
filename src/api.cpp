#include "api.hpp"
#include "core/cert_store.hpp"
#include "core/string/cow.hpp"
#include "generated/certificates.hpp"
#include "utils.hpp"

static iop::CertStore certStore(generated::certList);

#include "ESP8266httpUpdate.h"

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report CLIENT_BUFFER_OVERFLOWS

#ifndef IOP_API_DISABLED

static void upgradeScheduler() noexcept {
  utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
}

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
#ifdef IOP_ONLINE
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
  this->logger.debug(F("Report iop_panic:"), event.msg);

  const auto make = [event](JsonDocument &doc) {
    doc["file"] = event.file.get();
    doc["line"] = event.line;
    doc["func"] = event.func.get();
    doc["msg"] = event.msg.get();
  };
  // TODO: does every iop_panic possible fit into 2048 bytes?
  const auto maybeJson = this->makeJson<2048>(F("Api::reportPanic"), make);
  if (maybeJson.isNone())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  const auto token = authToken.asString();
  const auto maybeResp = this->network().httpPost(token, F("/panic"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = String(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::reportPanic: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
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
  const auto maybeJson = this->makeJson<256>(F("Api::registerEvent"), make);
  if (maybeJson.isNone())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  const auto token = authToken.asString();
  const auto maybeResp = this->network().httpPost(token, F("/event"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = String(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerEvent: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
#else
  return iop::NetworkStatus::OK;
#endif
}

auto Api::authenticate(iop::StringView username,
                       iop::StringView password) const noexcept
    -> iop::Result<AuthToken, iop::NetworkStatus> {
  IOP_TRACE();

  this->logger.debug(F("Authenticate IoP user: "), username);

  if (username.isEmpty() || password.isEmpty()) {
    this->logger.debug(F("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }
  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = std::move(username).get();
    doc["password"] = std::move(password).get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::authenticate"), make);
  if (maybeJson.isNone())
    return iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  auto maybeResp = this->network().httpPost(F("/user/login"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::authenticate: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto resp = UNWRAP_OK(maybeResp);

  if (resp.status != iop::NetworkStatus::OK) {
    return resp.status;
  }

  if (resp.payload.isNone()) {
    this->logger.error(F("Server answered OK, but payload is missing"));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto &payload = UNWRAP_REF(resp.payload);
  auto result = AuthToken::fromString(payload);
  if (IS_ERR(result)) {
    const auto lengthStr = std::to_string(payload.length());

    PROGMEM_STRING(parseErr, "Unprintable payload, this isn't supported: ");
    const auto &ref = UNWRAP_ERR(result);
    switch (ref) {
    case iop::ParseError::TOO_BIG:
      this->logger.error(F("Auth token is too big: size = "), lengthStr);
      return iop::NetworkStatus::BROKEN_SERVER;
    case iop::ParseError::NON_PRINTABLE:
      this->logger.error(parseErr,
                         iop::StringView(payload).scapeNonPrintable());
      return iop::NetworkStatus::BROKEN_SERVER;
    }
    this->logger.error(F("Unexpected ParseError: "),
                       String(static_cast<uint8_t>(ref)));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(result);
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      iop::StringView log) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = authToken.asString();
  this->logger.debug(F("Register log. Token: "), token, F(". Log: "), log);
  auto maybeResp = this->network().httpPost(token, F("/log"), std::move(log));

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = String(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerLog: "), code);
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(maybeResp).status;
#else
  return iop::NetworkStatus::OK;
#endif
}

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.debug(F("Upgrading sketch"));

  const auto tok = token.asString();
  const auto path = F("/update");
  const auto uri = String(this->uri().get()) + path;

  auto &client = iop::Network::wifiClient();

  ESPhttpUpdate.setAuthorization(String(tok.get()));
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
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    this->logger.error(F("Update failed: "),
                       ESPhttpUpdate.getLastErrorString());
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  // TODO(pc): properly handle ESPhttpUpdate.getLastError()
  this->logger.error(F("Update failed (UNKNOWN): "),
                     ESPhttpUpdate.getLastErrorString());
  return iop::NetworkStatus::BROKEN_SERVER;
}
#else
auto Api::loggerLevel() const noexcept -> iop::LogLevel {
  IOP_TRACE();
  return this->logger.level();
}
auto Api::upgrade(const AuthToken &token,
                  const MD5Hash &sketchHash) const noexcept
    -> iop::NetworkStatus {
  (void)*this;
  (void)token;
  (void)sketchHash;
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
    -> iop::Result<AuthToken, iop::NetworkStatus> {
  (void)*this;
  (void)std::move(username);
  (void)std::move(password);
  IOP_TRACE();
  return AuthToken::empty();
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

auto Api::loggerLevel() const noexcept -> iop::LogLevel {
  IOP_TRACE();
  return this->logger.level();
}
