#include <flash.hpp>
#include <EEPROM.h>

// TODO: use EEPROM.put and EEPROM.get
// https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/configManager.cpp
// TODO: diff before writing

#ifndef IOP_FLASH_DISABLED
#include <EEPROM.h>

// Flags to check if information is written to flash
const uint8_t usedWifiConfigEEPROMFlag = 123; // chosen by fair dice roll, garanteed to be random
const uint8_t usedAuthTokenEEPROMFlag = 124;
const uint8_t usedPlantIdEEPROMFlag = 125;


// Indexes so each function know where they can write to. It's kinda bad, but for now it works
const uint8_t wifiConfigIndex = 0;
const uint8_t wifiConfigSize = 1 + NetworkName().size() + NetworkPassword().size(); // Used flag + ssid + psk

const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint8_t authTokenSize = 1 + AuthToken().size(); // Used flag (1) + token

const uint8_t plantIdIndex = authTokenIndex + authTokenSize;

void Flash::setup() const {
  EEPROM.begin(512);
}

Option<PlantId> Flash::readPlantId() const {
  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag) {
    this->logger.debug(STATIC_STRING("No plant id stoder"));
    return Option<PlantId>();
  }

  PlantId id = {0};
  EEPROM.get(plantIdIndex + 1, id);
  this->logger.info(STATIC_STRING("Plant id found:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) id.data()), CONTINUITY);
  return Option<PlantId>(id);
}

void Flash::removePlantId() const {
  this->logger.info(STATIC_STRING("Deleting stored plant id"));
  if (EEPROM.read(plantIdIndex) == usedPlantIdEEPROMFlag) {
    EEPROM.write(plantIdIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writePlantId(const PlantId id) const {
  this->logger.info(STATIC_STRING("Writing plant id to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) id.data()), CONTINUITY);
  EEPROM.put(plantIdIndex, usedPlantIdEEPROMFlag);
  EEPROM.put(plantIdIndex + 1, id);
  EEPROM.commit();
}

Option<AuthToken> Flash::readAuthToken() const {
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag) {
    return Option<AuthToken>();
  }

  AuthToken token = {0};
  EEPROM.get(authTokenIndex + 1, token);
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const {
  this->logger.info(STATIC_STRING("Deleting stored auth token"));
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    EEPROM.write(authTokenIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken token) const {
  this->logger.info(STATIC_STRING("Writing auth token to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) token.data()), CONTINUITY);
  EEPROM.put(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenIndex + 1, token);
  EEPROM.commit();
}

Option<struct WifiCredentials> Flash::readWifiConfig() const {
  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag) {
    return Option<struct WifiCredentials>();
  }

  NetworkName ssid = {0};
  EEPROM.get(wifiConfigIndex + 1, ssid);
  NetworkPassword password = {0};
  EEPROM.get(wifiConfigIndex + 1 + ssid.size(), password);
  return Option<struct WifiCredentials>((struct WifiCredentials) { .ssid = ssid, .password = password });
}

void Flash::removeWifiConfig() const {
  this->logger.info(STATIC_STRING("Deleting stored wifi config"));
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    EEPROM.write(wifiConfigIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const struct WifiCredentials config) const {
  this->logger.info(STATIC_STRING("Writing wifi config to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) config.ssid.data()), CONTINUITY);

  EEPROM.put(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, config.ssid);
  EEPROM.put(wifiConfigIndex + 1 + NetworkName().size(), config.password);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() const {}
Option<AuthToken> Flash::readAuthToken() const { return Option<AuthToken>({0}); }
void Flash::removeAuthToken() const {}
void Flash::writeAuthToken(const AuthToken token) const { (void) token; }
Option<PlantId> Flash::readPlantId() const { return Option<PlantId>({0}); }
void Flash::removePlantId() const {};
void Flash::writePlantId(const PlantId id) const { (void) id; }
Option<struct WifiCredentials> Flash::readWifiConfig() const { return (struct WifiCredentials) {0}; }
void Flash::removeWifiConfig() const {}
void Flash::writeWifiConfig(const struct WifiCredentials id) const { (void) id; }
#endif
