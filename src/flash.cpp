#include "flash.hpp"

#ifdef IOP_DESKTOP
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

int read_(int fd, void* buf, size_t size) {
  return read(fd, buf, size);
}
int write_(int fd, const void* buf, size_t size) {
  return write(fd, buf, size);
}

class Eeprom {
  size_t size;
  std::unique_ptr<uint8_t[]> storage;
public:
  Eeprom(): size(0), storage(nullptr) {}

  void begin(size_t size) {
    if (size == 0) return;
    this->size = size;
    this->storage = iop::try_make_unique<uint8_t[]>(size);
    if (!this->storage) return;
    memset(this->storage.get(), '\0', size);
    
    const auto fd = open("eeprom.dat", O_RDONLY);
    if (fd == -1) return;
    if (read_(fd, this->storage.get(), size) == -1) return;
    close(fd);
  }
  uint8_t read(int const address) {
    if (address < 0 || address >= this->size) return 0;
    if (!this->storage) return 0;
    return this->storage[address];
  }
  void write(int const address, uint8_t const val) {
    if (address < 0 || address >= this->size) return;
    if (!this->storage) return;
    this->storage[address] = val;
  }
  bool commit() {
    const auto fd = open("eeprom.dat", O_WRONLY | O_CREAT, 0777);
    if (fd == -1) return false;
    if (!this->storage) return false;
    if (write_(fd, this->storage.get(), size) == -1) return false;
    if (close(fd) == -1) return false;
    return true;
  }
  bool end() {
    const auto ret = this->commit();
    this->storage.reset(nullptr);
    return ret;
  }

  uint8_t * getDataPtr() {
    return this->storage.get();
  }
  uint8_t const * getConstDataPtr() const {
    return this->storage.get();
  }

  template<typename T> 
  T &get(int const address, T &t) {
    if (address < 0 || address + sizeof(T) > _size)
      return t;

    memcpy((uint8_t*) &t, _data + address, sizeof(T));
    return t;
  }

  template<typename T> 
  const T &put(int const address, const T &t) {
    if (address < 0 || address + sizeof(T) > _size)
      return t;
    if (memcmp(_data + address, (const uint8_t*)&t, sizeof(T)) != 0) {
      _dirty = true;
      memcpy(_data + address, (const uint8_t*)&t, sizeof(T));
    }

    return t;
  }

  size_t length() {return _size;}

  uint8_t& operator[](int const address) {return getDataPtr()[address];}
  uint8_t const & operator[](int const address) const {return getConstDataPtr()[address];}

protected:
  uint32_t _sector;
  uint8_t* _data = nullptr;
  size_t _size = 0;
  bool _dirty = false;
};

static Eeprom EEPROM;
#else
#include "EEPROM.h"
#endif

#ifndef IOP_FLASH_DISABLED

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

static std::optional<AuthToken> authToken;
// Token cache

auto Flash::readAuthToken() const noexcept -> const std::optional<AuthToken> & {
  IOP_TRACE();

  // Checks if value is cached
  if (authToken.has_value())
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
  authToken.reset();

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
  if (maybeCurrToken.has_value()) {
    const auto &currToken = maybeCurrToken.value();

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
static std::optional<WifiCredentials> wifiCredentials;

auto Flash::readWifiConfig() const noexcept
    -> const std::optional<WifiCredentials> & {
  IOP_TRACE();

  // Check if value is in cache
  if (wifiCredentials.has_value())
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

  wifiCredentials.reset();

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
  if (maybeCurrConfig.has_value()) {
    const auto &currConfig = maybeCurrConfig.value();

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
auto Flash::readAuthToken() const noexcept -> const std::optional<AuthToken> & {
  (void)*this;
  IOP_TRACE();
  const static auto token = iop::some(AuthToken::empty());
  return token;
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

auto Flash::readWifiConfig() const noexcept -> const std::optional<WifiCredentials> & {
  (void)*this;
  IOP_TRACE();
  const static auto creds = iop::some(WifiCredentials(NetworkName::empty(), NetworkPassword::empty()));
  return creds;
}
void Flash::writeWifiConfig(const WifiCredentials &config) const noexcept {
  (void)*this;
  IOP_TRACE();
  (void)config;
}
#endif