#include <utils.hpp>
#include <api.hpp>
#include <network.hpp>
#include <models.hpp>
#include <flash.hpp>
#include <measurement.hpp>
#include <configuration.h>

#include <WiFiClient.h>
#include <array>

static Option<String> authToken = Option<String>();

AuthToken stringToAuthToken(const String &val) {
  unsigned int length = val.length();
  if (length > AuthToken().max_size()) {
    Log().crit("Auth token size returned is bigger than max size (" + String(length) + "): " + val);
    Log().warn("If it's bigger we will truncate the value, so it probably won't authenticate");
    length = AuthToken().max_size();
  }

  const uint8_t *payload = (uint8_t *) val.c_str();
  AuthToken token = {0};
  memcpy(token.data(), payload, length);
  return token;
}

PlantId stringToPlantId(const String &val) {
  unsigned int length = val.length();
  if (length > PlantId().max_size()) {
    Log().crit("Plant id size returned is bigger than max size (" + String(length) + "): " + val);
    Log().warn("If it's bigger we will truncate the value, so it probably won't find any");
    length = PlantId().max_size();
  }

  const uint8_t *payload = (uint8_t *) val.c_str();
  PlantId plantId = {0};
  memcpy(plantId.data(), payload, length);
  return plantId;
}

void measureAndSend(DHT &airTempAndHumidity, DallasTemperature &soilTemperature, const uint8_t soilResistivityPowerPin) {
  digitalWrite(LED_BUILTIN, HIGH);
  Option<AuthToken> authToken = readAuthTokenFromEEPROM();

  if (authToken.isNone()) {
    Log().crit("Auth token is none, but shouldn't be");
    return;
  }
  const String token = String((char*)authToken.unwrap().data());

  Option<PlantId> plantId = readPlantIdFromEEPROM();
  if (plantId.isNone()) {
    Log().crit("Plant id is none but shouldn't be");
    return;
  }

  const Event event = (Event) {
    .airTemperatureCelsius = measureAirTemperatureCelsius(airTempAndHumidity),
    .airHumidityPercentage = measureAirHumidityPercentage(airTempAndHumidity),
    .airHeatIndexCelsius = measureAirHeatIndexCelsius(airTempAndHumidity),
    .soilResistivityRaw = measureSoilResistivityRaw(soilResistivityPowerPin),
    .soilTemperatureCelsius = measureSoilTemperatureCelsius(soilTemperature),
    .plantId = String((char*)plantId.unwrap().data()),
  };

  sendEvent(token, event);
  digitalWrite(LED_BUILTIN, LOW);
}

void handleInterrupt() {
  const enum InterruptEvent event = interruptEvent;
  interruptEvent = NONE;

  switch (event) {
    case WPS:
      #ifndef IOP_ONLINE
        Log().warn("WPS doesn't work when WiFi is disabled by IOP_ONLINE undef");
      #endif

      #ifdef IOP_ONLINE
        Log().info("Connecting to WIFI through WPS");
        if (WiFi.beginWPSConfig()) {
          waitForConnection();
        } else {
          Log().error("WiFi.beginWPSConfig() returned false");
        }
      #endif
    case NONE:
      break;
  }
}

void panic__(const String msg, const String file, const int line, const String func) {
    Log().crit(msg);
    __panic_func(file.c_str(), line, func.c_str());
}

void Log::trace(const String msg) {
  log(TRACE, msg);
}

void Log::debug(const String msg) {
  log(DEBUG, msg);
}

void Log::info(const String msg) {
  log(INFO, msg);
}

void Log::warn(const String msg) {
  log(WARN, msg);
}

void Log::error(const String msg) {
  log(ERROR, msg);
}

void Log::crit(const String msg) {
  log(CRIT, msg);
}

void Log::log(const enum LogLevel level, const String msg) {
  #ifndef IOP_SERIAL
    return;
  #endif
  if (LOG_LEVEL > level) return;

  String levelName;
  switch (level) {
    case TRACE:
      levelName = "TRACE";
      break;
    case DEBUG:
      levelName = "DEBUG";
      break;
    case INFO:
      levelName = "INFO";
      break;
    case WARN:
      levelName = "WARN";
      break;
    case ERROR:
      levelName = "ERROR";
    case CRIT:
      levelName = "CRIT";
  }
  Serial.println("[" + levelName + "]: " + msg);
  Serial.flush();
}