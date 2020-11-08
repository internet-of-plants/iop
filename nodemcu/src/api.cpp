#include <api.hpp>

#include <ArduinoJson.h>

void Api::setup(std::function<void (const WiFiEventStationModeGotIP &)> onConnection) const {
  this->network.setup(onConnection);
}

Option<uint16_t> Api::registerEvent(const AuthToken authToken, const Event event) const { 
  char tokenChar[authToken.size() + 1] = {0};
  memcpy(&tokenChar, (char*) authToken.data(), authToken.size());
  this->logger.debug("Send event " + String(tokenChar));

  char idChar[event.plantId.size() + 1] = {0};
  memcpy(&idChar, (char*) event.plantId.data(), event.plantId.size());

  StaticJsonDocument<1048> doc;
  doc["air_temperature_celsius"] = event.airTemperatureCelsius;
  doc["air_humidity_percentage"] = event.airHumidityPercentage;
  doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
  doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
  doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  doc["plant_id"] = idChar;

  char buffer[1048];
  serializeJson(doc, buffer);
  auto maybeResp = this->network.httpPost(tokenChar, "/event", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    this->logger.error("Unable to make POST request to /event");
    #endif
    #endif
  }
  return maybeResp.map<uint16_t>([](const Response & resp) { return resp.code; });
}

Option<AuthToken> Api::authenticate(const String username, const String password) const {
  if (username.isEmpty() || password.isEmpty()) {
    return Option<AuthToken>();
  }

  this->logger.info("Generating token");

  StaticJsonDocument<300> doc;
  doc["email"] = username;
  doc["password"] = password;

  char buffer[300];
  serializeJson(doc, buffer);

  auto maybeResp = this->network.httpPost("/user/login", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    this->logger.error("Unable to make POST request to /user/login");
    return Option<AuthToken>();
    #else
    return Option<AuthToken>({0});
    #endif
    #else
    return Option<AuthToken>({0});
    #endif
  }

  return maybeResp.andThen<String>([](const Response &resp) { return resp.payload; })
    .andThen<AuthToken>([this](const String & val) {
      unsigned int length = val.length();
      if (length > AuthToken().max_size()) {
        this->logger.error("Auth token is too big: size = " + String(length));
        return Option<AuthToken>();
      }

      AuthToken token = {0};
      memcpy(token.data(), (uint8_t *) val.c_str(), token.size());
      return Option<AuthToken>(token);
    });
}

Result<PlantId, Option<uint16_t>> Api::registerPlant(const String token) const {
  this->logger.info("Get Plant Id. Token: " + token + ", MAC: " + this->macAddress());

  StaticJsonDocument<30> doc;
  doc["mac"] = this->macAddress();

  char buffer[30];
  serializeJson(doc, buffer);

  auto maybeResp = this->network.httpPut(token, "/plant", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    this->logger.error("Unable to make PUT request to /plant");
    #endif
    #endif
    return Result<PlantId, Option<uint16_t>>(Option<uint16_t>());
  }
  
  const auto resp = maybeResp.expect("Maybe resp is None");
  if (resp.code == 200) {
    unsigned int length = resp.payload.length();
    if (length > PlantId().max_size()) {
      logger.error("Plant id is too big: size = " + String(length));
      return Result<PlantId, Option<uint16_t>>(500);
    } else {
      const uint8_t *payload = (uint8_t *) resp.payload.c_str();
      PlantId plantId = {0};
      memcpy(plantId.data(), payload, plantId.size());
      return Result<PlantId, Option<uint16_t>>(plantId);
    }
  } else {
    return Result<PlantId, Option<uint16_t>>(resp.code);
  }
}