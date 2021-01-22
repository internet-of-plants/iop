#include "flash.hpp"

#ifndef IOP_FLASH_DISABLED
#include "EEPROM.h"

// Flags to check if information is written to flash
// chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 126;
const uint8_t usedAuthTokenEEPROMFlag = 127;

constexpr const uint16_t EEPROM_SIZE = 512;
// Indexes, so each function know where they can write to.
// It's kinda bad, but for now it works (TODO: maybe use FS.h?)
// There is always 1 bit used for the 'isWritten' flag
const uint8_t wifiConfigIndex = 0;
const uint8_t wifiConfigSize = 1 + NetworkName::size + NetworkPassword::size;

const uint8_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint8_t authTokenSize = 1 + AuthToken::size;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Flash::setup() noexcept -> void { EEPROM.begin(EEPROM_SIZE); }

static auto constPtr() noexcept -> const char * {
  // NOLINTNEXTLINE *-pro-type-reinterpret-cast
  return reinterpret_cast<const char *>(EEPROM.getConstDataPtr());
}

auto Flash::readAuthToken() const noexcept -> Option<AuthToken> {
  this->logger.debug(F("Reading AuthToken from Flash"));
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return Option<AuthToken>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + authTokenSize + 1;
  const auto token = AuthToken::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.debug(F("Auth token found: "), token.asString());
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const noexcept {
  this->logger.debug(F("Deleting stored auth token"));
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + authTokenIndex, 0, authTokenSize);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  this->logger.debug(F("Writing auth token to storage: "), token.asString());
  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenSize + 1, *token.asSharedArray());
  EEPROM.commit();
}

auto Flash::readWifiConfig() const noexcept -> Option<WifiCredentials> {
  this->logger.debug(F("Reading WifiCredentials from Flash"));

  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return Option<WifiCredentials>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + wifiConfigIndex + 1;
  const auto ssid = NetworkName::fromStringTruncating(UnsafeRawString(ptr));

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto pskRaw = UnsafeRawString(ptr + NetworkName::size);
  const auto psk = NetworkPassword::fromStringTruncating(pskRaw);

  this->logger.debug(F("Found network credentials: "), ssid.asString());
  return (WifiCredentials){.ssid = ssid, .password = psk};
}

void Flash::removeWifiConfig() const noexcept {
  this->logger.debug(F("Deleting stored wifi config"));
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + wifiConfigIndex, 0, wifiConfigSize);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  const auto ssidStr = config.ssid.asString();
  this->logger.debug(F("Writing network credentials to storage: "), ssidStr);
  const auto &psk = *config.password.asSharedArray();

  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, *config.ssid.asSharedArray());
  EEPROM.put(wifiConfigIndex + 1 + NetworkName::size, psk);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() const noexcept {}
Option<AuthToken> Flash::readAuthToken() const noexcept {
  return AuthToken::empty();
}
void Flash::removeAuthToken() const noexcept {}
void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  (void)token;
}
void Flash::removeWifiConfig() const noexcept {}
void Flash::writeWifiConfig(const struct WifiCredentials &id) const noexcept {
  (void)id;
}
#endif