#include <utils.hpp>
#include <api.hpp>
#include <network.hpp>
#include <models.hpp>
#include <flash.hpp>
#include <measurement.hpp>
#include <configuration.h>
#include <log.hpp>

#include <EEPROM.h>
#include <WiFiClient.h>
#include <array>

// TODO: we could also make a signinin possible via broadcast to the wifi network
// Although it would provide safety challenges, that maybe could be solved in a way similar to WPS
void handlePlantId(const AuthToken authToken, const String macAddress) {
  const String token = (char*) authToken.data();
  auto plantId = iopPlantId.map<PlantId>(stringToPlantId);

  if (plantId.isSome()) {
    const PlantId id = plantId.expect("Calling doWeOwnThisPlant, id is None but shouldn't be");
    const Option<bool> owner = api.doWeOwnsThisPlant(token, (char*) id.data());
    if (owner.isSome() && owner.expect("Owner is None")) {
      flash.writePlantId(id);
    }
  } else {
    plantId = api.registerPlant(token, macAddress);

    if (plantId.isSome()) {
      const PlantId id = plantId.expect("Calling writePlantId, id is None but shouldn't be");
      flash.writePlantId(id);
    } else {
      logger.error("Unable to get plant id");
    }
  }
}

AuthToken stringToAuthToken(const String &val) {
  unsigned int length = val.length();
  if (length > AuthToken().max_size()) {
    logger.crit("Auth token size returned is bigger than max size (" + String(length) + "): " + val);
    logger.warn("If it's bigger we will truncate the value, so it probably won't authenticate");
    length = AuthToken().max_size();
  }

  AuthToken token = {0};
  memcpy(token.data(), (uint8_t *) val.c_str(), length);
  return token;
}

PlantId stringToPlantId(const String &val) {
  unsigned int length = val.length();
  if (length > PlantId().max_size()) {
    logger.crit("Plant id size returned is bigger than max size (" + String(length) + "): " + val);
    logger.warn("If it's bigger we will truncate the value, so it probably won't find any");
    length = PlantId().max_size();
  }

  const uint8_t *payload = (uint8_t *) val.c_str();
  PlantId plantId = {0};
  memcpy(plantId.data(), payload, length);
  return plantId;
}

void handleInterrupt() {
  const enum InterruptEvent event = interruptEvent;
  interruptEvent = NONE;

  switch (event) {
    case NONE:
      break;
    case WPS:
      #ifndef IOP_ONLINE
        logger.warn("WPS doesn't work when WiFi is disabled by IOP_ONLINE undef");
        break;
      #endif

      #ifdef IOP_ONLINE
        logger.info("Connecting to WIFI through WPS");
        // TODO: There are some reports around WPS not working at WIFI_AP_STA mode, we should check that
        const WiFiMode_t mode = WiFi.getMode();
        WiFi.mode(WIFI_STA);
        if (WiFi.beginWPSConfig()) {
          WiFi.waitForConnectResult();
        } else {
          logger.error("WiFi.beginWPSConfig() returned false");
        }
        // WiFi Callbacks may race us
        if (WiFi.getMode() == WIFI_STA) {
          WiFi.mode(mode);
        }
        break;
      #endif
  }
}

void panic__(const String msg, const String file, const uint32_t line, const String func) {
  delay(1000);
  logger.crit("Panic at line " + String(line) + " of file " + file + ", inside " + func + ": " + msg);
  WiFi.mode(WIFI_OFF);
  // There should be a better way to black box this infinite loop
  while (EEPROM.read(0) != 3 && EEPROM.read(255) != 3) {
    yield();
  }
  __panic_func(file.c_str(), line, func.c_str());
}