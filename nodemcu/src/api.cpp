#include <ArduinoJson.h>

#include <api.hpp>
#include <configuration.h>

bool sendEvent(const String token, const Event event) {
  Serial.println("Send event " + token);

  StaticJsonDocument<2048> doc;
  doc["air_temperature_celsius"] = event.airTemperatureCelsius;
  doc["air_humidity_percentage"] = event.airHumidityPercentage;
  doc["air_heat_index_celsius"] = event.airHeatIndexCelsius;
  doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
  doc["soil_resistivity_raw"] = event.soilResistivityRaw;
  doc["plant_id"] = event.plantId;

  char buffer[2048];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPost(token, "/event", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    Serial.println("Unable to make POST request to /event");
    #endif
    return false;
  }

  const Response resp = maybeResp.unwrap();
  
  for (uint8_t count = 0; resp.code == 200 && count < 30; count++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }

  return resp.code == 200;
}

Option<String> generateToken() {
  Serial.print("Generating token ");

  StaticJsonDocument<300> doc;
  doc["email"] = iopEmail;
  doc["password"] = iopPassword;

  char buffer[300];
  serializeJson(doc, buffer);

  const Option<Response> maybeResp = httpPost("/user/login", String(buffer));

  if (maybeResp.isNone()) {
    #ifdef IOP_ONLINE
    Serial.println("Unable to make POST request to /user/login");
    #endif
    return Option<String>();
  }

  const Response resp = maybeResp.unwrap();

  for (uint8_t count = 0; resp.code == 200 && count < 30; count++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
  }

  return resp.payload;
}