#include <flash.hpp>

#include <EEPROM.h>

// These flags exist to check if the information is saved to EEPROM
// Conflicts basically will only happen at first boot
// And are not a problem, it will just try to login with invalid credentials and clear them
const uint8_t usedWifiConfigEEPROMFlag = 123; // chosen by fair dice roll, garanteed to be random
const uint8_t usedAuthTokenEEPROMFlag = 124;
const uint8_t usedPlantIdEEPROMFlag = 125;

const uint8_t wifiConfigIndex = 0;
const uint8_t authTokenIndex = wifiConfigIndex + 1 + 32 + 64 + 1 + 6;
const uint8_t plantIdIndex = authTokenIndex + 1 + AuthToken().max_size();

void Flash::setup() const {
  EEPROM.begin(512);
}

Option<PlantId> Flash::readPlantId() const {
  #ifndef IOP_MONITOR
    return Option<AuthToken>({0});
  #endif

  #ifdef IOP_MONITOR
  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag) {
    return Option<PlantId>();
  }

  PlantId id = {0};
  for (uint8_t index = 0; index < PlantId().max_size(); ++index) {
    id[index] = EEPROM.read(plantIdIndex + 1 + index);
  }
  return Option<PlantId>(id);
  #endif
}

void Flash::removePlantId() const {
  EEPROM.write(plantIdIndex, 0);
  EEPROM.commit();
}

void Flash::writePlantId(const PlantId id) const {
  EEPROM.write(plantIdIndex, usedPlantIdEEPROMFlag);
  for (uint8_t index = 0; index < PlantId().max_size(); ++index) {
    EEPROM.write(plantIdIndex + 1 + index, id[index]);
  }
  EEPROM.commit();
}

Option<AuthToken> Flash::readAuthToken() const {
  #ifndef IOP_MONITOR
    return Option<AuthToken>({0});
  #endif

  #ifdef IOP_MONITOR
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag) {
    return Option<AuthToken>();
  }

  AuthToken token = {0};
  for (uint8_t index = 0; index < AuthToken().max_size(); ++index) {
    token[index] = EEPROM.read(authTokenIndex + 1 + index);
  }
  return Option<AuthToken>(token);
  #endif
}

void Flash::removeAuthToken() const {
  EEPROM.write(authTokenIndex, 0);
  EEPROM.commit();
}

void Flash::writeAuthToken(const AuthToken token) const {
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
  // Maybe we should actually cleanup the credentials from flash
  // But the intention is to connect again as soon as possible, so they should be overriden
  // and the stale credentials should be invalid, so we don't (it would also be a waste of writes)
  EEPROM.write(usedWifiConfigEEPROMFlag, 0);
  EEPROM.commit();
}

void Flash::writeWifiConfig(const struct station_config config) const {
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
