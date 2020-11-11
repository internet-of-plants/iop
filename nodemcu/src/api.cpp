#include <api.hpp>

#ifndef IOP_API_DISABLED
#include <ArduinoJson.h>

void Api::setup() const {
  this->network.setup();
}

bool Api::isConnected() const { return this->network.isConnected(); }
String Api::macAddress() const { return this->network.macAddress(); }
void Api::disconnect() const { this->network.disconnect(); }
LogLevel Api::loggerLevel() const { return this->logger.level(); }

Option<HttpCode> Api::registerEvent(const AuthToken & authToken, const Event & event) const {
  this->logger.info(F("Send event"), START);

  static const auto makeJson = [](const Log &logger, const Event &event) {
    const auto id = event.plantId.asString();
    logger.info(id, CONTINUITY, F(" "));

    auto doc = std::unique_ptr<StaticJsonDocument<256>>(new StaticJsonDocument<256>());
    (*doc)["air_temperature_celsius"] = event.storage.airTemperatureCelsius;
    (*doc)["air_humidity_percentage"] = event.storage.airHumidityPercentage;
    (*doc)["air_heat_index_celsius"] = event.storage.airHeatIndexCelsius;
    (*doc)["soil_temperature_celsius"] = event.storage.soilTemperatureCelsius;
    (*doc)["soil_resistivity_raw"] = event.storage.soilResistivityRaw;
    (*doc)["plant_id"] = id.get();

    auto buffer = std::unique_ptr<std::array<char, 256>>(new std::array<char, 256>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };

  auto json = makeJson(this->logger, event);
  const auto token = authToken.asString();
  const auto maybeResp = this->network.httpPost(token, "/event", json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(F("Unable to make POST request to /event"));
  }
  return maybeResp.map<HttpCode>([](const Response & resp) { return resp.code; });
  #else
    return Option<HttpCode>(200);
  #endif
}

Result<AuthToken, Option<HttpCode>> Api::authenticate(const StringView username, const StringView password) const {
  if (username.isEmpty() || password.isEmpty()) {
    return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>(400));
  }

  this->logger.info(F("Generating token"));

  const auto makeJson = [](const Log & logger, const StringView username, const StringView password) {
    auto doc = std::unique_ptr<StaticJsonDocument<256>>(new StaticJsonDocument<256>());
    (*doc)["email"] = username.get();
    (*doc)["password"] = password.get();

    auto buffer = std::unique_ptr<std::array<char, 256>>(new std::array<char, 256>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };
  const auto json = makeJson(this->logger, username, password);
  auto maybeResp = this->network.httpPost(STATIC_STRING("/user/login"), json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(F("Unable to make POST request to /user/login"));
    return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>());
  } else {
    const auto resp = maybeResp.expect(F("maybeResp is none, inside Api::authenticate"));
    auto result = AuthToken::fromString(resp.payload);
    if (result.isErr()) {
      switch (result.expectErr(F("result isn't Err but should be"))) {
        case TOO_BIG:
          this->logger.error(F("Auth token is too big: size ="), START, F(" "));
          this->logger.error(String(resp.payload.length()));
          return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>(500));
          break;
      }
    }
    return Result<AuthToken, Option<HttpCode>>(result.expectOk(F("result isn't Ok but should be")));
  }
  #else
  return Result<AuthToken, Option<HttpCode>>(AuthToken((AuthToken::Storage) {0}));
  #endif
}

Result<PlantId, Option<HttpCode>> Api::registerPlant(const AuthToken & authToken) const {
  const auto makeJson = [](const Api & api) {
    auto doc = std::unique_ptr<StaticJsonDocument<30>>(new StaticJsonDocument<30>());
    (*doc)["mac"] = api.macAddress();

    auto buffer = std::unique_ptr<std::array<char, 30>>(new std::array<char, 30>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };
  const auto token = authToken.asString();
  const auto json = makeJson(*this);

  this->logger.info(F("Get Plant Id. Token:"), START, F(" "));
  this->logger.info(token, CONTINUITY, F(", "));
  this->logger.info(F("MAC:"), CONTINUITY, F(" "));
  this->logger.info(this->macAddress(), CONTINUITY);
  const auto maybeResp = this->network.httpPut(token, "/plant", json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(F("Unable to make POST request to /plant"));
    return Result<PlantId, Option<HttpCode>>(Option<HttpCode>());
  }

  const Response & resp = maybeResp.asRef().expect(F("Maybe resp is None"));
  if (resp.code == 200) {
    auto result = PlantId::fromString(resp.payload);
    if (result.isErr()) {
      switch (result.expectErr(F("result isn't Err but should be"))) {
        case TOO_BIG:
          this->logger.error(F("Auth token is too big: size ="), START, F(" "));
          this->logger.error(String(resp.payload.length()));
          return Result<PlantId, Option<HttpCode>>(Option<HttpCode>(500));
          break;
      }
    }
    return Result<PlantId, Option<HttpCode>>(result.expectOk(F("result isn't Ok but should be")));

  } else {
    return Result<PlantId, Option<HttpCode>>(resp.code);
  }
  #else
    return Result<PlantId, Option<HttpCode>>((PlantId) {0});
  #endif
}
#endif


#ifdef IOP_API_DISABLED
void Api::setup() const {
  this->network.setup();
}

bool Api::isConnected() const { return true; }
String Api::macAddress() const { return this->network->macAddress(); }
void Api::disconnect() const {}
LogLevel Api::loggerLevel() const { return this->logger.level(); }

Option<HttpCode> Api::registerEvent(const AuthToken & authToken, const Event & event) const {
  return Option<HttpCode>(200);
}

Result<AuthToken, Option<HttpCode>> Api::authenticate(const StringView username, const StringView password) const {
  return Result<AuthToken, Option<HttpCode>>(AuthToken((AuthToken::Storage) {0}));
}

Result<PlantId, Option<HttpCode>> Api::registerPlant(const AuthToken & authToken) const {
  return Result<PlantId, Option<HttpCode>>(PlantId((PlantId::Storage) {0}));
}
#endif
