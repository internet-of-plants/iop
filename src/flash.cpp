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
const uint16_t wifiConfigIndex = 0;
const uint16_t wifiConfigSize = 1 + NetworkName::size + NetworkPassword::size;

const uint16_t authTokenIndex = wifiConfigIndex + wifiConfigSize;
const uint16_t authTokenSize = 1 + AuthToken::size;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Flash::setup() noexcept -> void { EEPROM.begin(EEPROM_SIZE); }

static auto constPtr() noexcept -> const char * {
  IOP_TRACE();
  // NOLINTNEXTLINE *-pro-type-reinterpret-cast
  return reinterpret_cast<const char *>(EEPROM.getConstDataPtr());
}

static Option<AuthToken> authToken;

auto Flash::readAuthToken() const noexcept -> const Option<AuthToken> & {
  IOP_TRACE();

  if (authToken.isSome())
    return authToken;

  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return authToken;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + authTokenIndex + 1;
  const auto tokenRes = AuthToken::fromStringTruncating(UnsafeRawString(ptr));
  if (IS_ERR(tokenRes))
    return authToken;

  const auto &token = UNWRAP_OK_REF(tokenRes);
  this->logger.trace(F("Found Auth token: "), token.asString());

  authToken = token;
  return authToken;
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
  const auto &maybeCurrToken = this->readAuthToken();
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
  EEPROM.put(authTokenIndex + 1, *token.asSharedArray());
  EEPROM.commit();
}

static Option<WifiCredentials> wifiCredentials;

auto Flash::readWifiConfig() const noexcept -> const Option<WifiCredentials> & {
  IOP_TRACE();

  if (wifiCredentials.isSome())
    return wifiCredentials;

  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return wifiCredentials;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = constPtr() + wifiConfigIndex + 1;
  const auto ssidRes = NetworkName::fromStringTruncating(UnsafeRawString(ptr));
  if (IS_ERR(ssidRes))
    return wifiCredentials;
  const auto &ssid = UNWRAP_OK_REF(ssidRes);

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto pskRaw = UnsafeRawString(ptr + NetworkName::size);
  const auto pskRes = NetworkPassword::fromStringTruncating(pskRaw);
  if (IS_ERR(pskRes))
    return wifiCredentials;
  const auto &psk = UNWRAP_OK_REF(pskRes);

  this->logger.trace(F("Found network credentials: "), ssid.asString());
  wifiCredentials = WifiCredentials(ssid, psk);
  return wifiCredentials;
}

void Flash::removeWifiConfig() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored wifi config"));

  (void)wifiCredentials.take();
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
  const auto &maybeCurrConfig = this->readWifiConfig();
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