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
  IOP_TRACE();
  // NOLINTNEXTLINE *-pro-type-reinterpret-cast
  return reinterpret_cast<const char *>(EEPROM.getConstDataPtr());
}

static Option<AuthToken> authToken;

auto Flash::readAuthToken() const noexcept -> Option<AuthToken> {
  IOP_TRACE();
  this->logger.trace(F("Reading AuthToken from Flash"));

  if (authToken.isSome())
    return UNWRAP_REF(authToken);

  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return Option<AuthToken>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + authTokenSize + 1;
  const auto token = AuthToken::fromStringTruncating(UnsafeRawString(ptr));
  this->logger.trace(F("Auth token found: "), token.asString());

  authToken = token;
  return Option<AuthToken>(token);
}

void Flash::removeAuthToken() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored auth token"));

  (void)authToken.take();
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + authTokenIndex, 0, authTokenSize);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  IOP_TRACE();
  // Avoids re-writing same data
  const auto maybeCurrToken = this->readAuthToken();
  if (maybeCurrToken.isSome()) {
    const auto &currToken = UNWRAP_REF(maybeCurrToken);

    if (token.asString() == currToken.asString()) {
      this->logger.debug(F("Auth token already stored in flash"));
      // No need to save token that already is stored
      return;
    }
  }

  this->logger.info(F("Writing auth token to storage: "), token.asString());
  authToken = token;

  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenSize + 1, *token.asSharedArray());
  EEPROM.commit();
}

static Option<WifiCredentials> wifiCredentials;

auto Flash::readWifiConfig() const noexcept -> Option<WifiCredentials> {
  IOP_TRACE();
  this->logger.trace(F("Reading WifiCredentials from Flash"));

  if (wifiCredentials.isSome())
    return UNWRAP_REF(wifiCredentials);

  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return Option<WifiCredentials>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + wifiConfigIndex + 1;
  const auto ssid = NetworkName::fromStringTruncating(UnsafeRawString(ptr));

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto pskRaw = UnsafeRawString(ptr + NetworkName::size);
  const auto psk = NetworkPassword::fromStringTruncating(pskRaw);

  this->logger.trace(F("Found network credentials: "), ssid.asString());
  const WifiCredentials creds(ssid, psk);
  wifiCredentials = creds;
  return creds;
}

void Flash::removeWifiConfig() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored wifi config"));

  wifiCredentials.take();
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + wifiConfigIndex, 0, wifiConfigSize);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  IOP_TRACE();
  const auto ssidStr = config.ssid.asString();

  // Avoids re-writing same data
  const auto maybeCurrConfig = this->readWifiConfig();
  if (maybeCurrConfig.isSome()) {
    const auto &currConfig = UNWRAP_REF(maybeCurrConfig);

    if (currConfig.ssid.asString() == config.ssid.asString() &&
        currConfig.password.asString() == config.password.asString()) {
      this->logger.debug(F("WiFi credentials already stored in flash"));
      // No need to save credential that already are stored
      return;
    }
  }

  this->logger.info(F("Writing network credentials to storage: "), ssidStr);

  wifiCredentials = config;

  const auto &psk = *config.password.asSharedArray();

  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, *config.ssid.asSharedArray());
  EEPROM.put(wifiConfigIndex + 1 + NetworkName::size, psk);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() noexcept { IOP_TRACE(); }
auto Flash::readAuthToken() const noexcept -> Option<AuthToken> {
  (void)*this;
  IOP_TRACE();
  return AuthToken::empty();
}
void Flash::removeAuthToken() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)token;
}
void Flash::removeWifiConfig() const noexcept {
  (void)*this;
  IOP_TRACE();
}
auto Flash::readWifiConfig() const noexcept -> Option<WifiCredentials> {
  (void)*this;
  IOP_TRACE();
  return WifiCredentials(NetworkName::empty(), NetworkPassword::empty());
}
void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)config;
}
#endif