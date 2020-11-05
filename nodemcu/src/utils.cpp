#include <utils.hpp>
#include <api.hpp>
#include <network.hpp>
#include <models.hpp>
#include <flash.hpp>
#include <measurement.hpp>
#include <configuration.h>
#include <log.hpp>

#include <WiFiClient.h>
#include <array>

void handlePlantId(const AuthToken authToken, const String macAddress) {
  const String token = (char*) authToken.data();
  Option<PlantId> plantId = iopPlantId.map<PlantId>(stringToPlantId);

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
    case WPS:
      #ifndef IOP_ONLINE
        logger.warn("WPS doesn't work when WiFi is disabled by IOP_ONLINE undef");
      #endif

      #ifdef IOP_ONLINE
        logger.info("Connecting to WIFI through WPS");
        // TODO: there are some reports around WPS not working on WiFi mode WIFI_AP_STA
        // So we may need to force WIFI_STA mode to do that, and then return to WIFI_AP_STA mode
        // WIFI_AP mode is only needed when we can't login to a WiFi, so we become our own
        // access point, and provide a form to get the credentials. So if WPS works the AP is
        // shutdown anyway. The question is how fast can it recover from a change, and if we care
        // If WPS doesn't work the user will be kicked out of the WiFi if they are trying to fix through it
        // Although it's a rare case to care about, a user trying both methods at the same time...
        if (WiFi.beginWPSConfig()) {
          network.waitForConnection();
        } else {
          logger.error("WiFi.beginWPSConfig() returned false");
        }
      #endif
    case NONE:
      break;
  }
}

void panic__(const String msg, const String file, const int line, const String func) {
    logger.crit(msg);
    __panic_func(file.c_str(), line, func.c_str());
}
