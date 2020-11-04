#include <api.hpp>
#include <configuration.h>
#include <network.hpp>
#include <flash.hpp>

#include <ArduinoJson.h>

void apiSetup() {
  #ifndef IOP_ONLINE
    WiFi.mode(WIFI_OFF);
    authToken = Option<String>("4"); // chosen by fair dice roll, garanteed to be random
  #endif
  #ifndef IOP_MONITOR
    authToken = Option<String>("4"); // chosen by fair dice roll, garanteed to be random
  #endif
}

int sendEvent(const String token, const Event event) {
  Log().debug("Send event " + token);

  StaticJsonDocument<1048> doc;
  doc["air_temperature_celsius"] = event.airTemperatureCelsius;
  doc["air_humidity_percentage"] = event.airHumidityPercentage;
  doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
  doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
  doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  doc["plant_id"] = event.plantId;

  char buffer[1048];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPost(token, "/event", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    Log().error("Unable to make POST request to /event");
    #endif
    #endif
    return false;
  } else if (maybeResp.unwrap().code == 403) {
    Log().warn("Auth token was refused, deleting it");
    removeAuthTokenFromEEPROM();
  } else if (maybeResp.unwrap().code == 404) {
    Log().warn("Plant Id was not found, deleting it");
    removePlantIdFromEEPROM();
  }

  const Response resp = maybeResp.unwrap();
  return resp.code == 200;
}

Option<String> responseToMaybeString(const Response &resp) {
  return resp.payload;
}

bool doWeOwnsThisPlant(const String token, const String plantId) {
  Log().info("Check if we own plant. Token = " + token + ", Plant Id = " + plantId);

  StaticJsonDocument<30> doc;
  doc["id"] = plantId;

  char buffer[30];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPost("/plant/owns", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    Log().error("Unable to make POST request to /plant/owns");
    #endif
    #endif
    return true; // We don't know, so let's not delete a possibly correct id just yet
  } else if (maybeResp.unwrap().code == 404) {
    return false;
  } else if (maybeResp.unwrap().code == 403) {
    Log().warn("Auth token was refused, deleting it");
    removeAuthTokenFromEEPROM();
    return true; // We actually don't know yet, but we can't know without fixing this auth problem
  } else {
    return true;
  }
}

Option<AuthToken> generateToken(const String username, const String password) {
  Log().info("Generating token");

  StaticJsonDocument<300> doc;
  doc["email"] = iopEmail.expect("No iop email available");
  doc["password"] = iopPassword.expect("No iop password available");

  char buffer[300];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPost("/user/login", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    Log().error("Unable to make POST request to /user/login");
    #endif
    #endif
    return Option<AuthToken>();
  }

  return maybeResp.andThen<String>(responseToMaybeString)
            .map<AuthToken>(stringToAuthToken);
}

Option<PlantId> getPlantId(const String token, const String macAddress) {
  Log().info("Get Plant Id. Token: " + token + ", MAC: " + macAddress);

  StaticJsonDocument<30> doc;
  doc["mac"] = macAddress;

  char buffer[30];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPut(token, "/plant", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    #ifdef IOP_MONITOR
    Log().error("Unable to make PUT request to /plant");
    #endif
    #endif
    return Option<PlantId>();
  } else if (maybeResp.unwrap().code == 403) {
    Log().warn("Auth token was refused, deleting it");
    removeAuthTokenFromEEPROM();
    return Option<PlantId>();
  } else {
    return maybeResp.andThen<String>(responseToMaybeString)
      .map<PlantId>(stringToPlantId);
  }
}