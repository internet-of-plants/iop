#include "flash.hpp"
#include "loop.hpp"

#ifndef IOP_FLASH_DISABLED
#include "driver/flash.hpp"
#include "core/panic.hpp"

constexpr const uint16_t EEPROM_SIZE = 512;

// If another type is to be written to flash be carefull not to mess with what
// already is there and update the static_assert below. Same deal for removing
// types.

// Magic bytes. Flags to check if information is written to flash.
// Chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 126;
const uint8_t usedAuthTokenEEPROMFlag = 127;

// One byte is reserved for the magic byte ('isWritten' flag)
const uint16_t authTokenSize = 1 + 64;
const uint16_t wifiConfigSize = 1 + 32 + 64;

// Allows each method to know where to write
const uint16_t wifiConfigIndex = 0;
const uint16_t authTokenIndex = wifiConfigIndex + wifiConfigSize;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Flash::setup() noexcept -> void { driver::flash.setup(EEPROM_SIZE); }

static bool cachedAuthToken = false;
auto Flash::readAuthToken() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  IOP_TRACE();

  // Checks if value is cached
  if (cachedAuthToken)
    return std::make_optional(std::ref(unused4KbSysStack.token()));

  // Check if magic byte is set in flash (as in, something is stored)
  if (driver::flash.read(authTokenIndex) != usedAuthTokenEEPROMFlag)
    return std::optional<std::reference_wrapper<const AuthToken>>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const uint8_t *ptr = driver::flash.asRef() + authTokenIndex + 1;
  
  // Updates cache
  memcpy(unused4KbSysStack.token().data(), ptr, 64);

  const auto tok = std::string_view(unused4KbSysStack.token().data(), 64);
  // AuthToken must be printable US-ASCII (to be stored in HTTP headers))
  if (!iop::isAllPrintable(tok)) {
    this->logger.error(F("Auth token was non printable: "), iop::to_view(iop::scapeNonPrintable(tok)));
    this->removeAuthToken();
    return std::optional<std::reference_wrapper<const AuthToken>>();
  }

  this->logger.trace(F("Found Auth token: "), tok);

  cachedAuthToken = true;
  return std::make_optional(std::ref(unused4KbSysStack.token()));
}

void Flash::removeAuthToken() const noexcept {
  IOP_TRACE();

  // Clears cache, if any
  cachedAuthToken = false;
  unused4KbSysStack.token().fill('\0');

  // Checks if it's written to flash first, avoids wasting writes
  if (driver::flash.read(authTokenIndex) == usedAuthTokenEEPROMFlag) {
    this->logger.info(F("Deleting stored auth token"));

    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::flash.asMut() + authTokenIndex, 0, authTokenSize);
    driver::flash.commit();
  }
}

void Flash::writeAuthToken(const AuthToken &token) const noexcept {
  IOP_TRACE();

  // Avoids re-writing the same data
  const auto maybeCurrToken = this->readAuthToken();
  if (maybeCurrToken.has_value()) {
    const auto &currToken = iop::unwrap_ref(maybeCurrToken, IOP_CTX()).get();

    if (memcmp(token.data(), currToken.data(), currToken.max_size()) == 0) {
      this->logger.debug(F("Auth token already stored in flash"));
      return;
    }
  }

  this->logger.info(F("Writing auth token to storage: "), std::string_view(token.data(), token.max_size()));

  // Updates cache
  cachedAuthToken = true;
  unused4KbSysStack.token() = token;

  driver::flash.write(authTokenIndex, usedAuthTokenEEPROMFlag);
  driver::flash.put(authTokenIndex + 1, token);
  driver::flash.commit();
}

static bool cachedSSID = false;
auto Flash::readWifiConfig() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  IOP_TRACE();

  // Check if value is in cache
  if (cachedSSID)
    return std::make_optional(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));

  // Check if magic byte is set in flash (as in, something is stored)
  if (driver::flash.read(wifiConfigIndex) != usedWifiConfigEEPROMFlag)
    return std::optional<std::reference_wrapper<const WifiCredentials>>();

  // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
  const auto *ptr = driver::flash.asRef() + wifiConfigIndex + 1;

  // We treat wifi credentials as a blob instead of worrying about encoding

  memcpy(unused4KbSysStack.ssid().data(), ptr, 32);
  memcpy(unused4KbSysStack.psk().data(), ptr + 32, 64);

  const auto ssidStr = iop::scapeNonPrintable(std::string_view(unused4KbSysStack.ssid().data(), 32));
  this->logger.trace(F("Found network credentials: "), iop::to_view(ssidStr));

  // Clears cache, if any
  cachedSSID = true;
  return std::make_optional(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));
}

void Flash::removeWifiConfig() const noexcept {
  IOP_TRACE();
  this->logger.info(F("Deleting stored wifi config"));

  cachedSSID = false;
  unused4KbSysStack.ssid().fill('\0');
  unused4KbSysStack.psk().fill('\0');

  // Checks if it's written to flash first, avoids wasting writes
  if (driver::flash.read(wifiConfigIndex) == usedWifiConfigEEPROMFlag) {
    // NOLINTNEXTLINE *-pro-bounds-pointer-arithmetic
    memset(driver::flash.asMut() + wifiConfigIndex, 0, wifiConfigSize);
    driver::flash.commit();
  }
}

void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const auto &maybeCurrConfig = this->readWifiConfig();
  if (maybeCurrConfig.has_value()) {
    const auto &currConfig = iop::unwrap_ref(maybeCurrConfig, IOP_CTX()).get();

    if (memcmp(currConfig.ssid.get().begin(), config.ssid.get().begin(), 32) == 0 &&
        memcmp(currConfig.password.get().begin(), config.password.get().begin(), 64) == 0) {
      this->logger.debug(F("WiFi credentials already stored in flash"));
      // No need to save credential that already are stored
      return;
    }
  }

  this->logger.info(F("Writing network credentials to storage: "), std::string_view(config.ssid.get().data(), 32));

  // Updates cache
  cachedAuthToken = true;
  unused4KbSysStack.ssid() = config.ssid.get();
  unused4KbSysStack.psk() = config.password.get();

  driver::flash.write(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  driver::flash.put(wifiConfigIndex + 1, config.ssid);
  driver::flash.put(wifiConfigIndex + 1 + 32, config.password);
  driver::flash.commit();
}
#endif

#ifdef IOP_FLASH_DISABLED
void Flash::setup() noexcept { IOP_TRACE(); }
auto Flash::readAuthToken() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  (void)*this;
  IOP_TRACE();
  return std::make_optional(AuthToken(unused4KbSysStack.token()));
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

auto Flash::readWifiConfig() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  (void)*this;
  IOP_TRACE();
  return std::make_optional(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));
}
void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)config;
}
#endif