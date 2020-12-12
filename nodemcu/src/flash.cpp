#include <flash.hpp>
#include <EEPROM.h>

#ifndef IOP_FLASH_DISABLED
#include <EEPROM.h>

// Flags to check if information is written to flash
constexpr const uint8_t usedWifiConfigEEPROMFlag = 126; // chosen by fair dice roll, garanteed to be random
constexpr const uint8_t usedAuthTokenEEPROMFlag = 127;
constexpr const uint8_t usedPlantIdEEPROMFlag = 128;


// Indexes so each function know where they can write to. It's kinda bad, but for now it works
constexpr const uint8_t wifiConfigIndex = 0;
constexpr const uint8_t wifiConfigSize = 1 + NetworkName::size + NetworkPassword::size; // Used flag + ssid + psk

constexpr const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
constexpr const uint8_t authTokenSize = 1 + AuthToken::size; // Used flag (1) + token

constexpr const uint8_t plantIdIndex = authTokenIndex + authTokenSize;

void Flash::setup() const noexcept {
  EEPROM.begin(512);
}

Option<PlantId> Flash::readPlantId() const noexcept {
  if (EEPROM.read(plantIdIndex) != usedPlantIdEEPROMFlag) {
    return Option<PlantId>();
  }

  const auto ptr = (char *) EEPROM.getConstDataPtr() + plantIdIndex + 1;
  const auto id = PlantId::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.info(F("Plant id found:"), START, F(" "));
  this->logger.info(id.asString(), CONTINUITY);
  return Option<PlantId>(id);
}

void Flash::removePlantId() const noexcept {
  this->logger.info(F("Deleting stored plant id"));
  if (EEPROM.read(plantIdIndex) == usedPlantIdEEPROMFlag) {
    EEPROM.write(plantIdIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writePlantId(const PlantId & id) const noexcept {
  this->logger.info(F("Writing plant id to storage:"), START, F(" "));
  this->logger.info(id.asString(), CONTINUITY);
  EEPROM.write(plantIdIndex, usedPlantIdEEPROMFlag);
  EEPROM.put(plantIdIndex + 1, *PlantId(id).intoInner().intoInner().get());
  EEPROM.commit();
}

Option<AuthToken> Flash::readAuthToken() const noexcept {
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag) {
    return Option<AuthToken>();
  }

  const auto ptr = (char *) EEPROM.getConstDataPtr() + authTokenSize + 1;
  const auto token = AuthToken::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.info(F("Auth token found:"), START, F(" "));
  this->logger.info(token.asString(), CONTINUITY);
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const noexcept {
  this->logger.info(F("Deleting stored auth token"));
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    EEPROM.write(authTokenIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken & token) const noexcept {
  this->logger.info(F("Writing auth token to storage:"), START, F(" "));
  this->logger.info(token.asString(), CONTINUITY);
  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenSize + 1, *AuthToken(token).intoInner().intoInner().get());
  EEPROM.commit();
}

Option<struct WifiCredentials> Flash::readWifiConfig() const noexcept {
  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag) {
    return Option<struct WifiCredentials>();
  }

  const auto ptr = (char *) EEPROM.getConstDataPtr() + wifiConfigIndex + 1;
  const auto ssid = NetworkName::fromStringTruncating(UnsafeRawString(ptr));
  const auto psk = NetworkPassword::fromStringTruncating(UnsafeRawString(ptr + NetworkName::size));
  this->logger.info(F("Found wifi credentials for network "), START, F(" "));
  this->logger.info(ssid.asString());
  return Option<struct WifiCredentials>((struct WifiCredentials) { .ssid = ssid, .password = psk });
}

void Flash::removeWifiConfig() const noexcept {
  this->logger.info(F("Deleting stored wifi config"));
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    EEPROM.write(wifiConfigIndex, 0);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const struct WifiCredentials & config) const noexcept {
  this->logger.info(F("Writing wifi config to storage:"), START, F(" "));
  this->logger.info(config.ssid.asString(), CONTINUITY);

  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, NetworkName(config.ssid).intoInner().intoInner().get());
  EEPROM.put(wifiConfigIndex + 1 + NetworkName::size, NetworkPassword(config.password).intoInner().intoInner().get());
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() const noexcept {}
Option<AuthToken> Flash::readAuthToken() const noexcept { return Option<AuthToken>(AuthToken::empty()); }
void Flash::removeAuthToken() const noexcept {}
void Flash::writeAuthToken(const AuthToken & token) const noexcept { (void) token; }
Option<PlantId> Flash::readPlantId() const noexcept { return Option<PlantId>(PlantId::empty()); }
void Flash::removePlantId() const noexcept {};
void Flash::writePlantId(const PlantId & id) const noexcept { (void) id; }
Option<struct WifiCredentials> Flash::readWifiConfig() const noexcept { return (struct WifiCredentials) {
  .ssid = NetworkName(NetworkName::empty()),
  .password = NetworkPassword(NetworkPassword::empty()),
}; }
void Flash::removeWifiConfig() const noexcept {}
void Flash::writeWifiConfig(const struct WifiCredentials & id) const noexcept { (void) id; }
#endif