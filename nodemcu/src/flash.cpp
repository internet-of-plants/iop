#include <flash.hpp>
#include <EEPROM.h>

// TODO: use EEPROM.put and EEPROM.get, also try only commit once
// (all information generally can be retrieved together)
// https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/configManager.cpp

#ifndef IOP_FLASH_DISABLED
#include <EEPROM.h>

// Flags to check if information is written to flash
const uint8_t usedWifiConfigEEPROMFlag = 123; // chosen by fair dice roll, garanteed to be random
const uint8_t usedAuthTokenEEPROMFlag = 124;
const uint8_t usedPlantIdEEPROMFlag = 125;


// Indexes so each function know where they can write to. It's kinda bad, but for now it works
const uint8_t wifiConfigIndex = 0;
const uint8_t wifiConfigSize = 1 + 32 + 64 + 1 + 6; // Used flag(1) + ssid(32) + psk(64) + bssid flag(1) + bssid(6)

const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint8_t authTokenSize = 1 + AuthToken().max_size(); // Used flag (1) + token

const uint8_t plantIdIndex = authTokenIndex + authTokenSize;

void Flash::setup() const {
  EEPROM.begin(512);
}

Option<PlantId> Flash::readPlantId() const {
  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag) {
    return Option<PlantId>();
  }

  PlantId id = {0};
  for (uint8_t index = 0; index < PlantId().max_size(); ++index) {
    id[index] = EEPROM.read(plantIdIndex + 1 + index);
  }
  this->logger.info(STATIC_STRING("Plant id found:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) id.data()), CONTINUITY);
  return Option<PlantId>(id);
}

void Flash::removePlantId() const {
  this->logger.info(STATIC_STRING("Deleting stored plant id"));
  EEPROM.write(plantIdIndex, 0);
  EEPROM.commit();
}

void Flash::writePlantId(const PlantId id) const {
  this->logger.info(STATIC_STRING("Writing plant id to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) id.data()), CONTINUITY);
  EEPROM.write(plantIdIndex, usedPlantIdEEPROMFlag);
  for (uint8_t index = 0; index < PlantId().max_size(); ++index) {
    EEPROM.write(plantIdIndex + 1 + index, id[index]);
  }
  EEPROM.commit();
}

Option<AuthToken> Flash::readAuthToken() const {
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag) {
    return Option<AuthToken>();
  }

  AuthToken token = {0};
  for (uint8_t index = 0; index < AuthToken().max_size(); ++index) {
    token[index] = EEPROM.read(authTokenIndex + 1 + index);
  }
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const {
  this->logger.info(STATIC_STRING("Deleting stored auth token"));
  EEPROM.write(authTokenIndex, 0);
  EEPROM.commit();
}

void Flash::writeAuthToken(const AuthToken token) const {
  this->logger.info(STATIC_STRING("Writing auth token to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*) token.data()), CONTINUITY);
  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  for (uint8_t index = 0; index < AuthToken().max_size(); ++index) {
    EEPROM.write(authTokenIndex + 1 + index, token[index]);
  }
  EEPROM.commit();
}

Option<struct station_config> Flash::readWifiConfig() const {
  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag) {
    return Option<struct station_config>();
  }

  struct station_config config = {0};
  for (uint8_t index = 0; index < 32; ++index) {
    config.ssid[index] = EEPROM.read(wifiConfigIndex + 1 + index);
  }
  for (uint8_t index = 0; index < 64; ++index) {
    config.password[index] = EEPROM.read(wifiConfigIndex + 1 + 32 + index);
  }
  config.bssid_set = EEPROM.read(wifiConfigIndex + 1 + 32 + 64);
  if (config.bssid_set) {
    for (uint8_t index = 0; index < 6; ++index) {
      config.bssid[index] = EEPROM.read(wifiConfigIndex + 1 + 32 + 64 + index);
    }
  }
  return Option<struct station_config>(config);
}

void Flash::removeWifiConfig() const {
  this->logger.info(STATIC_STRING("Deleting stored wifi config"));
  EEPROM.write(usedWifiConfigEEPROMFlag, 0);
  EEPROM.commit();
}

void Flash::writeWifiConfig(const struct station_config config) const {
  this->logger.info(STATIC_STRING("Writing wifi config to storage:"), START, STATIC_STRING(" "));
  this->logger.info(StaticString((char*)config.ssid), CONTINUITY);
  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  for (uint8_t index = 0; index < 32; ++index) {
    EEPROM.write(wifiConfigIndex + 1 + index, config.ssid[index]);
  }
  for (uint8_t index = 0; index < 64; ++index) {
    EEPROM.write(wifiConfigIndex + 1 + 32 + index, config.password[index]);
  }
  EEPROM.write(1 + 32 + 64, config.bssid_set);
  if (config.bssid_set) {
    for (uint8_t index = 0; index < 6; ++index) {
      EEPROM.write(wifiConfigIndex + 1 + 32 + 64 + 1 + index, config.bssid[index]);
    }
  }
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
Option<struct station_config> Flash::readWifiConfig() const { return (station_config) {0}; }
void Flash::removeWifiConfig() const {}
void Flash::writeWifiConfig(const struct station_config id) const { (void) id; }
#endif
