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

Option<uint16_t> Api::registerEvent(const AuthToken authToken, const Event event) const { 
  char tokenChar[authToken.size() + 1] = {0};
  memcpy(&tokenChar, (char*) authToken.data(), authToken.size());
  this->logger.info("Send event " + String(tokenChar));

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
  const auto maybeResp = this->network.httpPost(tokenChar, "/event", String(buffer));

  if (maybeResp.isNone()) {
    //this->logger.error("Unable to make POST request to /event");
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

  const auto maybeResp = this->network.httpPost("/user/login", String(buffer));

  if (maybeResp.isNone()) {
    this->logger.error("Unable to make POST request to /user/login");
  }

  return maybeResp.andThen<String>([](const Response &resp) { return resp.payload; })
    .andThen<AuthToken>([this](const String & val) {
      if (val.length() > AuthToken().max_size()) {
        this->logger.error("Auth token is too big: size = " + String(val.length()));
        return Option<AuthToken>();
      }

      AuthToken token = {0};
      memcpy(token.data(), (uint8_t *) val.c_str(), val.length());
      return Option<AuthToken>(token);
    });
}

Result<PlantId, Option<uint16_t>> Api::registerPlant(const String token) const {
  this->logger.info("Get Plant Id. Token: " + token + ", MAC: " + this->macAddress());

  StaticJsonDocument<30> doc;
  doc["mac"] = this->macAddress();

  char buffer[30];
  serializeJson(doc, buffer);

  const auto maybeResp = this->network.httpPut(token, "/plant", String(buffer));

  if (maybeResp.isNone()) {
    this->logger.error("Unable to make POST request to /plant");
  }
  
  const Response & resp = maybeResp.asRef().expect("Maybe resp is None");
  if (resp.code == 200) {
    const unsigned int length = resp.payload.length();
    if (length > PlantId().max_size()) {
      this->logger.error("Plant id is too big: size = " + String(length));
      return Result<PlantId, Option<uint16_t>>(500);
    } else {
      const uint8_t *payload = (uint8_t *) resp.payload.c_str();
      PlantId plantId = {0};
      memcpy(plantId.data(), payload, length);
      return Result<PlantId, Option<uint16_t>>(plantId);
    }
  } else {
    return Result<PlantId, Option<uint16_t>>(resp.code);
  }
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

Option<uint16_t> Api::registerEvent(const AuthToken authToken, const Event event) const { 
  return Option<uint16_t>(200);
}

Option<AuthToken> Api::authenticate(const String username, const String password) const {
  return Option<AuthToken>({0});
}

Result<PlantId, Option<uint16_t>> Api::registerPlant(const String token) const {
  return Result<PlantId, Option<uint16_t>>((PlantId) {0});
}
#endif