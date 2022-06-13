#include "iop/storage.hpp"
#include "iop/loop.hpp"
#include <optional>

#include "iop-hal/storage.hpp"
#include "iop-hal/panic.hpp"

namespace iop {
constexpr const uintmax_t EEPROM_SIZE = 512;

// If another type is to be written to storage be carefull not to mess with what
// already is there and update the static_assert below. Same deal for removing
// types.

// Magic bytes. Flags to check if information is written to storage.
// Chosen by fair dice roll, garanteed to be random
const uint8_t usedWifiConfigEEPROMFlag = 125;
const uint8_t usedAuthTokenEEPROMFlag = 126;

// One byte is reserved for the magic byte ('isWritten' flag)
const uintmax_t authTokenSize = 1 + 64;
const uintmax_t wifiConfigSize = 1 + 32 + 64;

// Allows each method to know where to write
const uintmax_t wifiConfigIndex = 0;
const uintmax_t authTokenIndex = wifiConfigIndex + wifiConfigSize;

static_assert(authTokenIndex + authTokenSize < EEPROM_SIZE,
              "EEPROM too small to store needed credentials");

auto Storage::setup() noexcept -> void { iop_hal::storage.setup(EEPROM_SIZE); }

static AuthToken authToken;

auto Storage::token() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>> {
  IOP_TRACE();

  // Check if magic byte is set in storage (as in, something is stored)
  const auto flag = iop_hal::storage.get(authTokenIndex);
  if (!flag || *flag != usedAuthTokenEEPROMFlag)
    return std::nullopt;

  const auto maybeAuthToken = iop_hal::storage.read<sizeof(AuthToken)>(authTokenIndex + 1);
  iop_assert(maybeAuthToken, IOP_STR("Failed to read AuthToken from storage"));
  authToken = *maybeAuthToken;
  
  const auto tok = iop::to_view(authToken);
  // AuthToken must be printable US-ASCII (to be stored in HTTP headers))
  if (!iop::isAllPrintable(tok) || tok.length() != 64) {
    this->logger.error(IOP_STR("Auth token was non printable: "), iop::to_view(iop::scapeNonPrintable(tok)));
    this->removeToken();
    return std::nullopt;
  }

  this->logger.trace(IOP_STR("Found Auth token: "), tok);
  const auto ref = std::reference_wrapper<const AuthToken>(authToken);
  return std::make_optional(ref);
}

void Storage::removeToken() const noexcept {
  IOP_TRACE();

  authToken.fill('\0');

  // Checks if it's written to storage first, avoids wasting writes
  const auto flag = iop_hal::storage.get(authTokenIndex);
  if (flag && *flag == usedAuthTokenEEPROMFlag) {
    this->logger.info(IOP_STR("Deleting stored auth token"));

    iop_hal::storage.set(authTokenIndex, 0);
    iop_hal::storage.write(authTokenIndex + 1, authToken);
    iop_hal::storage.commit();
  }
}

void Storage::setToken(const AuthToken &token) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const auto flag = iop_hal::storage.get(authTokenIndex);
  if (flag && *flag == usedAuthTokenEEPROMFlag) {
    const auto tok = iop_hal::storage.read<sizeof(AuthToken)>(authTokenIndex + 1);
    iop_assert(tok, IOP_STR("Failed to read AuthToken from storage"));

    if (*tok == token) {
      this->logger.debug(IOP_STR("Auth token already stored in storage"));
      return;
    }
  }

  this->logger.info(IOP_STR("Writing auth token to storage: "), iop::to_view(token));
  iop_hal::storage.set(authTokenIndex, usedAuthTokenEEPROMFlag);
  iop_hal::storage.write(authTokenIndex + 1, token);
  iop_hal::storage.commit();
}

static iop::NetworkName ssid;
static iop::NetworkPassword psk;

auto Storage::wifi() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>> {
  IOP_TRACE();

  // Check if magic byte is set in storage (as in, something is stored)
  const auto flag = iop_hal::storage.get(wifiConfigIndex);
  if (!flag || *flag != usedWifiConfigEEPROMFlag)
    return std::nullopt;

  const auto maybeSsid = iop_hal::storage.read<sizeof(iop::NetworkName)>(wifiConfigIndex + 1);
  const auto maybePsk = iop_hal::storage.read<sizeof(iop::NetworkPassword)>(wifiConfigIndex + 1);
  iop_assert(maybeSsid, IOP_STR("Failed to read SSID from storage"));
  iop_assert(maybePsk, IOP_STR("Failed to read PSK from storage"));

  ssid = *maybeSsid;
  psk = *maybePsk;
  
  const auto ssidStr = iop::scapeNonPrintable(iop::to_view(ssid));
  this->logger.trace(IOP_STR("Found network credentials: "), iop::to_view(ssidStr));
  const auto creds = WifiCredentials(ssid, psk);
  return std::make_optional(std::ref(creds));
}

void Storage::removeWifi() const noexcept {
  IOP_TRACE();
  this->logger.info(IOP_STR("Deleting stored wifi config"));

  ssid.fill('\0');
  psk.fill('\0');

  // Checks if it's written to storage first, avoids wasting writes
  const auto flag = iop_hal::storage.get(wifiConfigIndex);
  if (flag && *flag == usedWifiConfigEEPROMFlag) {
    iop_hal::storage.set(wifiConfigIndex, 0);
    iop_hal::storage.write(wifiConfigIndex + 1, ssid);
    iop_hal::storage.write(wifiConfigIndex + sizeof(iop::NetworkName) + 1, psk);
    iop_hal::storage.commit();
  }
}

void Storage::setWifi(const WifiCredentials &config) const noexcept {
  IOP_TRACE();

  // Avoids re-writing same data
  const auto flag = iop_hal::storage.get(wifiConfigIndex);
  if (flag && *flag == usedWifiConfigEEPROMFlag) {
    const auto networkName = iop_hal::storage.read<sizeof(iop::NetworkName)>(wifiConfigIndex + 1);
    const auto networkPassword = iop_hal::storage.read<sizeof(iop::NetworkPassword)>(wifiConfigIndex + sizeof(iop::NetworkName) + 1);
    iop_assert(networkName, IOP_STR("Failed to read SSID from storage"));
    iop_assert(networkPassword, IOP_STR("Failed to read PSK from storage"));
    
    // Theoretically SSIDs can have a nullptr inside of it, but currently ESP8266 gives us random garbage after the first '\0' instead of zeroing the rest
    // So we do not accept SSIDs with a nullptr in the middle
    if (iop::to_view(*networkName) == iop::to_view(config.ssid.get()) && iop::to_view(*networkPassword) == iop::to_view(config.password.get())) {
      this->logger.debug(IOP_STR("Wifi Credentials already stored in storage"));
      return;
    }
  }
  
  this->logger.info(IOP_STR("Writing wifi credentials to storage: "), iop::to_view(config.ssid.get()));
  this->logger.debug(IOP_STR("WiFi Creds: "), iop::to_view(config.ssid.get()), IOP_STR(" "), iop::to_view(config.password.get()));

  iop_hal::storage.set(wifiConfigIndex, usedWifiConfigEEPROMFlag);
  iop_hal::storage.write(wifiConfigIndex + 1, config.ssid.get());
  iop_hal::storage.write(wifiConfigIndex + sizeof(iop::NetworkName) + 1, config.password.get());
  iop_hal::storage.commit();
}
}