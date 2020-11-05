#include <api.hpp>
#include <configuration.h>
#include <network.hpp>
#include <flash.hpp>
#include <log.hpp>

#include <ArduinoJson.h>

void Api::setup() const {
  #ifndef IOP_ONLINE
    authToken = Option<String>("4"); // chosen by fair dice roll, garanteed to be random
  #endif
  #ifndef IOP_MONITOR
    authToken = Option<String>("4"); // chosen by fair dice roll, garanteed to be random
  #endif
}

bool Api::registerEvent(const AuthToken authToken, const Event event) const {
  const String token = (char*) authToken.data();
  logger.debug("Send event " + token);

  StaticJsonDocument<1048> doc;
  doc["air_temperature_celsius"] = event.airTemperatureCelsius;
  doc["air_humidity_percentage"] = event.airHumidityPercentage;
  doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
  doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
  doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  doc["plant_id"] = event.plantId;

  char buffer[1048];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = network.httpPost(token, "/event", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    logger.error("Unable to make POST request to /event");
    #endif
    #endif
    return false;
  } else if (maybeResp.expect("Maybe resp is None 1").code == 403) {
    logger.warn("Auth token was refused, deleting it");
    flash.removeAuthToken();
  } else if (maybeResp.expect("Maybe resp is None 2").code == 404) {
    logger.warn("Plant Id was not found, deleting it");
    flash.removePlantId();
  }

  const Response resp = maybeResp.expect("Maybe resp is None 3");
  return resp.code == 200;
}

Option<String> responseToMaybeString(const Response &resp) {
  return resp.payload;
}

Option<bool> Api::doWeOwnsThisPlant(const String token, const String plantId) const {
  logger.info("Check if we own plant. Token = " + token + ", Plant Id = " + plantId);

  StaticJsonDocument<30> doc;
  doc["id"] = plantId;

  char buffer[30];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = network.httpPost("/plant/owns", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    logger.error("Unable to make POST request to /plant/owns");
    #endif
    #endif
    return Option<bool>();
  } else if (maybeResp.expect("Maybe resp is None 4").code == 404) {
    return Option<bool>(false);
  } else if (maybeResp.expect("Maybe resp is None 5").code == 403) {
    logger.warn("Auth token was refused, deleting it");
    flash.removeAuthToken();
    return Option<bool>();
  } else {
    return Option<bool>(true);
  }
}

Option<AuthToken> Api::authenticate(const String username, const String password) const {
  logger.info("Generating token");

  StaticJsonDocument<300> doc;
  doc["email"] = iopEmail.expect("No iop email available");
  doc["password"] = iopPassword.expect("No iop password available");

  char buffer[300];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = network.httpPost("/user/login", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    logger.error("Unable to make POST request to /user/login");
    #endif
    #endif
    return Option<AuthToken>();
  }

  return maybeResp.andThen<String>(responseToMaybeString)
            .map<AuthToken>(stringToAuthToken);
}

Option<PlantId> Api::registerPlant(const String token, const String macAddress) const {
  logger.info("Get Plant Id. Token: " + token + ", MAC: " + macAddress);

  StaticJsonDocument<30> doc;
  doc["mac"] = macAddress;

  char buffer[30];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = network.httpPut(token, "/plant", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    logger.error("Unable to make PUT request to /plant");
    #endif
    #endif
    return Option<PlantId>();
  } else if (maybeResp.expect("Maybe resp is None").code == 403) {
    logger.warn("Auth token was refused, deleting it");
    flash.removeAuthToken();
    return Option<PlantId>();
  } else {
    return maybeResp.andThen<String>(responseToMaybeString)
      .map<PlantId>(stringToPlantId);
  }
}
