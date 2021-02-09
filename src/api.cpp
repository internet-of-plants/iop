#include "api.hpp"
#include "core/string/cow.hpp"

#include "ESP8266httpUpdate.h"

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report CLIENT_BUFFER_OVERFLOWS

#ifndef IOP_API_DISABLED

auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept -> ApiStatus {
  IOP_TRACE();
  this->logger.debug(F("Report panic_hook:"), event.msg);

  const auto make = [event](JsonDocument &doc) {
    doc["file"] = event.file.get();
    doc["line"] = event.line;
    doc["func"] = event.func.get();
    doc["msg"] = event.msg.get();
  };
  // TODO: does every panic_hook possible fit into 2048 bytes?
  const auto maybeJson = this->makeJson<2048>(F("Api::reportPanic"), make);
  if (maybeJson.isNone())
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  const auto token = authToken.asString();
  const auto maybeResp =
      this->network().httpPost(token, F("/panic_hook"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = String(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::reportPanic: "), code);
    return ApiStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

auto Api::registerEvent(const AuthToken &authToken,
                        const Event &event) const noexcept -> ApiStatus {
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
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  const auto token = authToken.asString();
  const auto maybeResp = this->network().httpPost(token, F("/event"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = String(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerEvent: "), code);
    return ApiStatus::BROKEN_SERVER;
  }
  return UNWRAP_OK_REF(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

auto Api::authenticate(iop::StringView username,
                       iop::StringView password) const noexcept
    -> iop::Result<AuthToken, ApiStatus> {
  IOP_TRACE();

  this->logger.debug(F("Authenticate IoP user: "), username);

  if (username.isEmpty() || password.isEmpty()) {
    this->logger.debug(F("Empty username or password, at Api::authenticate"));
    return ApiStatus::FORBIDDEN;
  }
  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = std::move(username).get();
    doc["password"] = std::move(password).get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::authenticate"), make);
  if (maybeJson.isNone())
    return ApiStatus::CLIENT_BUFFER_OVERFLOW;
  const auto &json = UNWRAP_REF(maybeJson);

  auto maybeResp = this->network().httpPost(F("/user/login"), json);

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::authenticate: "), code);
    return ApiStatus::BROKEN_SERVER;
  }

  const auto resp = UNWRAP_OK(maybeResp);

  if (resp.status != ApiStatus::OK) {
    return resp.status;
  }

  if (resp.payload.isNone()) {
    this->logger.error(F("Server answered OK, but payload is missing"));
    return ApiStatus::BROKEN_SERVER;
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
      return ApiStatus::BROKEN_SERVER;
    case iop::ParseError::NON_PRINTABLE:
      this->logger.error(parseErr,
                         iop::StringView(payload).scapeNonPrintable());
      return ApiStatus::BROKEN_SERVER;
    }
    this->logger.error(F("Unexpected ParseError: "),
                       String(static_cast<uint8_t>(ref)));
    return ApiStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(result);
#else
  return AuthToken::empty();
#endif
}

auto Api::registerLog(const AuthToken &authToken,
                      iop::StringView log) const noexcept -> ApiStatus {
  IOP_TRACE();
  const auto token = authToken.asString();
  this->logger.debug(F("Register log. Token: "), token, F(". Log: "), log);
  // TODO(pc): dynamically generate the log string, instead of buffering it,
  // similar to a Stream but we choose when to write (using logger template
  // variadics)
  //
  // Network::httpPost sets `Content-Type` to `application/json` is there is a
  // payload
  auto maybeResp = this->network().httpPost(token, F("/log"), std::move(log));

#ifndef IOP_MOCK_MONITOR
  if (IS_ERR(maybeResp)) {
    const auto code = std::to_string(UNWRAP_ERR_REF(maybeResp));
    this->logger.error(F("Unexpected response at Api::registerLog: "), code);
    return ApiStatus::BROKEN_SERVER;
  }

  return UNWRAP_OK(maybeResp).status;
#else
  return ApiStatus::OK;
#endif
}

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

auto Api::upgrade(const AuthToken &token, const MacAddress &mac,
                  const MD5Hash &sketchHash) const noexcept -> ApiStatus {
  IOP_TRACE();
  this->logger.debug(F("Upgrading sketch"));

  const auto tok = token.asString();

  String path(F("/update/"));
  path += tok.get();
  path += F("/");
  path += mac.asString().get();

  auto &client = Network::wifiClient();

  const auto *const version = sketchHash.asString().get();
  // TODO: upstream we already can use the setAuthorization header, but it's not
  // published
  const auto uri = String(this->uri().get()) + path;

  ESPhttpUpdate.closeConnectionsOnUpdate(true);
  ESPhttpUpdate.rebootOnUpdate(true);
  ESPhttpUpdate.setLedPin(LED_BUILTIN);
  const auto updateResult = ESPhttpUpdate.updateFS(client, uri, version);

#ifdef IOP_MOCK_MONITOR
  return ApiStatus::OK;
#endif

  switch (updateResult) {
  case HTTP_UPDATE_NO_UPDATES:
  case HTTP_UPDATE_OK:
    return ApiStatus::OK;
  case HTTP_UPDATE_FAILED:
    // TODO(pc): properly handle ESPhttpUpdate.getLastError()
    this->logger.error(F("Update failed: "),
                       ESPhttpUpdate.getLastErrorString());
    return ApiStatus::BROKEN_SERVER;
  }
  // TODO(pc): properly handle ESPhttpUpdate.getLastError()
  this->logger.error(F("Update failed (UNKNOWN): "),
                     ESPhttpUpdate.getLastErrorString());
  return ApiStatus::BROKEN_SERVER;
}
#else
auto Api::loggerLevel() const noexcept -> iop::LogLevel {
  IOP_TRACE();
  return this->logger.level();
}
auto Api::upgrade(const AuthToken &token,
                  const MD5Hash &sketchHash) const noexcept -> ApiStatus {
  (void)*this;
  (void)token;
  (void)sketchHash;
  IOP_TRACE();
  return ApiStatus::OK;
}
auto Api::reportPanic(const AuthToken &authToken,
                      const PanicData &event) const noexcept -> ApiStatus {
  (void)*this;
  (void)authToken;
  (void)event;
  IOP_TRACE();
  return ApiStatus::OK;
}
auto Api::registerEvent(const AuthToken &token,
                        const Event &event) const noexcept -> ApiStatus {
  (void)*this;
  (void)token;
  (void)event;
  IOP_TRACE();
  return ApiStatus::OK;
}
auto Api::authenticate(iop::StringView username,
                       iop::StringView password) const noexcept
    -> iop::Result<AuthToken, ApiStatus> {
  (void)*this;
  (void)std::move(username);
  (void)std::move(password);
  IOP_TRACE();
  return AuthToken::empty();
}
auto Api::registerLog(const AuthToken &authToken,
                      iop::StringView log) const noexcept -> ApiStatus {
  (void)*this;
  (void)authToken;
  (void)std::move(log);
  IOP_TRACE();
  return ApiStatus::OK;
}
#endif

auto Api::setup() const noexcept -> void {
  IOP_TRACE();
  this->network().setup();
}

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

auto Api::network() const noexcept -> const Network & {
  IOP_TRACE();
  return this->network_;
}

auto Api::loggerLevel() const noexcept -> iop::LogLevel {
  IOP_TRACE();
  return this->logger.level();
}