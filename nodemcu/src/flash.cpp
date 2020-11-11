#include <flash.hpp>
#include <EEPROM.h>

#ifndef IOP_FLASH_DISABLED
#include <EEPROM.h>

// Flags to check if information is written to flash
const uint8_t usedWifiConfigEEPROMFlag = 126; // chosen by fair dice roll, garanteed to be random
const uint8_t usedAuthTokenEEPROMFlag = 127;
const uint8_t usedPlantIdEEPROMFlag = 128;


// Indexes so each function know where they can write to. It's kinda bad, but for now it works
const uint8_t wifiConfigIndex = 0;
const uint8_t wifiConfigSize = 1 + (uint8_t) NetworkName::dataSize + (uint8_t) NetworkPassword::dataSize; // Used flag + ssid + psk

const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint8_t authTokenSize = 1 + (uint8_t) AuthToken::dataSize; // Used flag (1) + token

const uint8_t plantIdIndex = authTokenIndex + authTokenSize;

void Flash::setup() const {
  EEPROM.begin(512);
}

Option<PlantId> Flash::readPlantId() const {
  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag) {
    this->logger.debug(F("No plant id stoder"));
    return Option<PlantId>();
  }

  PlantId id((PlantId::Storage) {0});
  memcpy(id.mutPtr(), EEPROM.getConstDataPtr() + plantIdIndex + 1, PlantId::dataSize);
  this->logger.info(F("Plant id found:"), START, F(" "));
  this->logger.info(id.asString(), CONTINUITY);
  return Option<PlantId>(id);
}

void Flash::removePlantId() const {
  this->logger.info(F("Deleting stored plant id"));
  if (EEPROM.read(plantIdIndex) == usedPlantIdEEPROMFlag) {
    EEPROM.write(plantIdIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writePlantId(const PlantId & id) const {
  this->logger.info(F("Writing plant id to storage:"), START, F(" "));
  this->logger.info(id.asString(), CONTINUITY);
  EEPROM.write(plantIdIndex, usedPlantIdEEPROMFlag);
  memcpy(EEPROM.getDataPtr() + plantIdIndex + 1, id.constPtr(), PlantId::dataSize);
  EEPROM.commit();
}

Option<AuthToken> Flash::readAuthToken() const {
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag) {
    return Option<AuthToken>();
  }

  AuthToken token((AuthToken::Storage) {0});
  memcpy(token.mutPtr(), EEPROM.getConstDataPtr() + authTokenIndex + 1, AuthToken::dataSize);
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const {
  this->logger.info(F("Deleting stored auth token"));
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    EEPROM.write(authTokenIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken & token) const {
  this->logger.info(F("Writing auth token to storage:"), START, F(" "));
  this->logger.info(token.asString(), CONTINUITY);
  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  memcpy(EEPROM.getDataPtr() + authTokenIndex + 1, token.constPtr(), AuthToken::dataSize);
  EEPROM.commit();
}

Option<struct WifiCredentials> Flash::readWifiConfig() const {
  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag) {
    return Option<struct WifiCredentials>();
  }

  NetworkName ssid((NetworkName::Storage) {0});
  memcpy(ssid.mutPtr(), EEPROM.getConstDataPtr() + wifiConfigIndex + 1, NetworkName::dataSize);
  NetworkPassword psk((NetworkPassword::Storage) {0});
  memcpy(psk.mutPtr(), EEPROM.getConstDataPtr() + wifiConfigIndex + 1 + NetworkName::dataSize, NetworkPassword::dataSize);
  return Option<struct WifiCredentials>((struct WifiCredentials) { .ssid = ssid, .password = psk });
}

void Flash::removeWifiConfig() const {
  this->logger.info(F("Deleting stored wifi config"));
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    EEPROM.write(wifiConfigIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const struct WifiCredentials & config) const {
  this->logger.info(F("Writing wifi config to storage:"), START, F(" "));
  this->logger.info(config.ssid.asString(), CONTINUITY);

  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  memcpy(EEPROM.getDataPtr() + wifiConfigIndex + 1, config.ssid.constPtr(), NetworkName::dataSize);
  memcpy(EEPROM.getDataPtr() + wifiConfigIndex + 1 + NetworkName::dataSize, config.password.constPtr(), NetworkPassword::dataSize);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() const {}
Option<AuthToken> Flash::readAuthToken() const { return Option<AuthToken>((AuthToken::Storage) {0}); }
void Flash::removeAuthToken() const {}
void Flash::writeAuthToken(const AuthToken & token) const { (void) token; }
Option<PlantId> Flash::readPlantId() const { return Option<PlantId>((PlantId::Storage) {0}); }
void Flash::removePlantId() const {};
void Flash::writePlantId(const PlantId & id) const { (void) id; }
Option<struct WifiCredentials> Flash::readWifiConfig() const { return (struct WifiCredentials) {
  .ssid = NetworkName((NetworkName::Storage) {0}),
  .password = NetworkPassword((NetworkPassword::Storage) {0}),
}; }
void Flash::removeWifiConfig() const {}
void Flash::writeWifiConfig(const struct WifiCredentials & id) const { (void) id; }
#endif
