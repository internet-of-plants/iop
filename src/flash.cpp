#include "flash.hpp"

#ifndef IOP_FLASH_DISABLED
#include "EEPROM.h"
#include "core/string/cow.hpp"

constexpr const uint16_t EEPROM_SIZE = 512;

// If another type is to be written to flash be carefull not to mess with what
// already is there and update the static_assert below. Same deal for removing
// types.

// Magic bytes. Flags to check if information is written to flash.
// Chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 126;
const uint8_t usedAuthTokenEEPROMFlag = 127;

// One byte is reserved for the magic byte ('isWritten' flag)
const uint16_t authTokenSize = 1 + AuthToken::size;
const uint16_t wifiConfigSize = 1 + NetworkName::size + NetworkPassword::size;

// Allows each method to know where to write
const uint16_t wifiConfigIndex = 0;
const uint16_t authTokenIndex = wifiConfigIndex + wifiConfigSize;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Flash::setup() noexcept -> void { EEPROM.begin(EEPROM_SIZE); }

static iop::Option<AuthToken> authToken;
// Token cache

auto Flash::readAuthToken() const noexcept -> const iop::Option<AuthToken> & {
  IOP_TRACE();

  // Checks if value is cached
  if (authToken.isSome())
    return authToken;

  // Check if magic byte is set in flash (as in, something is stored)
  if (EEPROM.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return authToken;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto ptr = EEPROM.getConstDataPtr() + authTokenIndex + 1;

  auto token = AuthToken::fromBytesUnsafe(ptr, AuthToken::size);

  const auto tok = token.asString();
  // AuthToken must be printable US-ASCII (to be stored in HTTP headers))
  if (!token.isAllPrintable()) {
    this->logger.error(F("Auth token was non printable: "), tok);
    this->removeAuthToken();
    return authToken;
  }

  this->logger.trace(F("Found Auth token: "), tok);

  // Updates cache
  authToken.emplace(std::move(token));
  return authToken;
}

void Flash::removeAuthToken() const noexcept {
  IOP_TRACE();

  // Clears cache, if any
  (void)authToken.take();

  // Checks if it's written to flash first, avoids wasting writes
  if (EEPROM.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    this->logger.info(F("Deleting stored auth token"));

    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + authTokenIndex, 0, authTokenSize);
    EEPROM.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  IOP_TRACE();

  // Avoids re-writing the same data
  const auto &maybeCurrToken = this->readAuthToken();
  if (maybeCurrToken.isSome()) {
    const auto &currToken = UNWRAP_REF(maybeCurrToken);

    if (memcmp(token.constPtr(), currToken.constPtr(), AuthToken::size) == 0) {
      this->logger.debug(F("Auth token already stored in flash"));
      return;
    }
  }

  this->logger.info(F("Writing auth token to storage: "), token.asString());

  // Updates cache
  authToken.emplace(token);

  EEPROM.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  EEPROM.put(authTokenIndex + 1, *token.asSharedArray());
  EEPROM.commit();
}

// Credentials cache
static iop::Option<WifiCredentials> wifiCredentials;

auto Flash::readWifiConfig() const noexcept
    -> const iop::Option<WifiCredentials> & {
  IOP_TRACE();

  // Check if value is in cache
  if (wifiCredentials.isSome())
    return wifiCredentials;

  // Check if magic byte is set in flash (as in, something is stored)
  if (EEPROM.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return wifiCredentials;

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto *ptr = EEPROM.getConstDataPtr() + wifiConfigIndex + 1;

  // We treat wifi credentials as a blob instead of worrying about encoding

  const auto ssid = NetworkName::fromBytesUnsafe(ptr, NetworkName::size);
  const auto psk = NetworkPassword::fromBytesUnsafe(ptr, NetworkPassword::size);

  const auto ssidStr = ssid.asString();
  this->logger.trace(F("Found network credentials: "), ssidStr);

  // Updates cache
  wifiCredentials.emplace(ssid, psk);
  return wifiCredentials;
}

void Flash::removeWifiConfig() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored wifi config"));

  // Clears cache, if any
  (void)wifiCredentials.take();

  // Checks if it's written to flash first, avoids wasting writes
  if (EEPROM.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(EEPROM.getDataPtr() + wifiConfigIndex, 0, wifiConfigSize);
    EEPROM.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const auto &maybeCurrConfig = this->readWifiConfig();
  if (maybeCurrConfig.isSome()) {
    const auto &currConfig = UNWRAP_REF(maybeCurrConfig);

    if (memcmp(currConfig.ssid.constPtr(), config.ssid.constPtr(),
               NetworkName::size) == 0 &&
        memcmp(currConfig.password.constPtr(), config.password.constPtr(),
               NetworkPassword::size) == 0) {
      this->logger.debug(F("WiFi credentials already stored in flash"));
      // No need to save credential that already are stored
      return;
    }
  }

  this->logger.info(F("Writing network credentials to storage: "),
                    config.ssid.asString());

  // Updates cache
  wifiCredentials.emplace(config);

  const auto &psk = *config.password.asSharedArray();
  EEPROM.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  EEPROM.put(wifiConfigIndex + 1, *config.ssid.asSharedArray());
  EEPROM.put(wifiConfigIndex + 1 + NetworkName::size, psk);
  EEPROM.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() noexcept { IOP_TRACE(); }
auto Flash::readAuthToken() const noexcept -> iop::Option<AuthToken> {
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
auto Flash::readWifiConfig() const noexcept -> iop::Option<WifiCredentials> {
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