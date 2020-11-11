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
  this->logger.info(STATIC_STRING("Send event"), START, STATIC_STRING(" "));

  static const auto makeJson = [](const Log &logger, const Event &event) {
    constexpr const uint8_t idSize = PlantId().size() + 1;
    auto idChar = std::unique_ptr<std::array<char, idSize>>(new std::array<char, idSize>);
    idChar->fill(0);
    memcpy(idChar->data(), (char*) event.plantId.data(), event.plantId.size());
    logger.info(StaticString(idChar->data()), CONTINUITY, STATIC_STRING(" "));

    auto doc = std::unique_ptr<StaticJsonDocument<256>>(new StaticJsonDocument<256>());
    (*doc)["air_temperature_celsius"] = event.airTemperatureCelsius;
    (*doc)["air_humidity_percentage"] = event.airHumidityPercentage;
    (*doc)["air_heat_index_celsius"] = event.airHeatIndexCelsius;
    (*doc)["soil_temperature_celsius"] = event.soilTemperatureCelsius;
    (*doc)["soil_resistivity_raw"] = event.soilResistivityRaw;
    (*doc)["plant_id"] = idChar->data();

    auto buffer = std::unique_ptr<std::array<char, 256>>(new std::array<char, 256>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };

  static const auto makeToken = [](const Log & logger, const AuthToken & authToken) {
    constexpr const uint8_t tokenSize = AuthToken().size() + 1;
    auto tokenChar = std::unique_ptr<std::array<char, tokenSize>>(new std::array<char, tokenSize>);
    tokenChar->fill(0);
    memcpy(tokenChar->data(), (char*) authToken.data(), authToken.size());
    const String token = String(tokenChar->data());
    logger.info(token, CONTINUITY);
    return token;
  };

  auto json = makeJson(this->logger, event);
  const auto token = makeToken(this->logger, authToken);
  const auto maybeResp = this->network.httpPost(token, "/event", json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(STATIC_STRING("Unable to make POST request to /event"));
  }
  return maybeResp.map<HttpCode>([](const Response & resp) { return resp.code; });
  #else
    return Option<HttpCode>(200);
  #endif
}

Result<AuthToken, Option<HttpCode>> Api::authenticate(const String & username, const String & password) const {
  if (username.isEmpty() || password.isEmpty()) {
    return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>());
  }

  this->logger.info(STATIC_STRING("Generating token"));

  const auto makeJson = [](const Log & logger, const String & username, const String & password) {
    auto doc = std::unique_ptr<StaticJsonDocument<256>>(new StaticJsonDocument<256>());
    (*doc)["email"] = username;
    (*doc)["password"] = password;

    auto buffer = std::unique_ptr<std::array<char, 256>>(new std::array<char, 256>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };
  const auto json = makeJson(this->logger, username, password);
  auto maybeResp = this->network.httpPost("/user/login", json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(STATIC_STRING("Unable to make POST request to /user/login"));
    return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>());
  } else {
    const auto resp = maybeResp.expect(STATIC_STRING("maybeResp is none, inside Api::authenticate"));
    if (resp.payload.length() > AuthToken().size()) {
      this->logger.error(STATIC_STRING("Auth token is too big: size ="), START, STATIC_STRING(" "));
      this->logger.error(String(resp.payload.length()));
      return Result<AuthToken, Option<HttpCode>>(Option<HttpCode>());
    }
    AuthToken token = {0};
    memcpy(token.data(), (uint8_t *) resp.payload.c_str(), resp.payload.length());
    return Result<AuthToken, Option<HttpCode>>(token);
  }
  #else
  return Result<AuthToken, Option<HttpCode>>({0});
  #endif
}

Result<PlantId, Option<HttpCode>> Api::registerPlant(const AuthToken & authToken) const {
  static const auto makeToken = [](const Log & logger, const AuthToken & authToken) {
    constexpr const uint8_t tokenSize = AuthToken().size() + 1;
    auto tokenChar = std::unique_ptr<std::array<char, tokenSize>>(new std::array<char, tokenSize>);
    tokenChar->fill(0);
    memcpy(tokenChar->data(), (char*) authToken.data(), authToken.size());
    const String token = String(tokenChar->data());
    logger.info(token);
    return token;
  };

  const auto makeJson = [](const Api & api) {
    auto doc = std::unique_ptr<StaticJsonDocument<30>>(new StaticJsonDocument<30>());
    (*doc)["mac"] = api.macAddress();

    auto buffer = std::unique_ptr<std::array<char, 30>>(new std::array<char, 30>());
    buffer->fill(0);
    serializeJson(*doc, buffer->data(), buffer->size());
    auto json = String(buffer->data());
    return json;
  };
  const auto token = makeToken(this->logger, authToken);
  const auto json = makeJson(*this);

  this->logger.info(STATIC_STRING("Get Plant Id. Token:"), START, STATIC_STRING(" "));
  this->logger.info(token, CONTINUITY, STATIC_STRING(", "));
  this->logger.info(STATIC_STRING("MAC:"), CONTINUITY, STATIC_STRING(" "));
  this->logger.info(this->macAddress(), CONTINUITY);
  const auto maybeResp = this->network.httpPut(token, "/plant", json);

  #ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    this->logger.error(STATIC_STRING("Unable to make POST request to /plant"));
  }

  const Response & resp = maybeResp.asRef().expect(STATIC_STRING("Maybe resp is None"));
  if (resp.code == 200) {
    const unsigned int length = resp.payload.length();
    if (length > PlantId().max_size()) {
      this->logger.error(STATIC_STRING("Plant id is too big: size ="), START, STATIC_STRING(" "));
      this->logger.error(String(length), CONTINUITY);
      return Result<PlantId, Option<HttpCode>>(500);
    } else {
      const uint8_t *payload = (uint8_t *) resp.payload.c_str();
      PlantId plantId = {0};
      memcpy(plantId.data(), payload, length);
      return Result<PlantId, Option<HttpCode>>(plantId);
    }
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
String Api::macAddress() const { return "MAC::MAC::ERS::ON"; }
void Api::disconnect() const { }
LogLevel Api::loggerLevel() const { return TRACE; }

Option<HttpCode> Api::registerEvent(const AuthToken & authToken, const Event & event) const {
  return Option<HttpCode>(200);
}

Option<AuthToken> Api::authenticate(const String & username, const String & password) const {
  return Option<AuthToken>({0});
}

Result<PlantId, Option<HttpCode>> Api::registerPlant(const AuthToken & token) const {
  return Result<PlantId, Option<HttpCode>>((PlantId) {0});
}
#endif
