#ifndef IOP_FLASH_H_
#define IOP_FLASH_H_

#include <ESP8266WiFi.h>

#include <models.hpp>
#include <option.hpp>
#include <log.hpp>

/// Wraps flash memory to provide a API that satisfies our storage needs
class Flash {
  Log logger;

  public:
    Flash(const LogLevel logLevel) noexcept: logger(logLevel, F("FLASH")) {}
    Flash(Flash& other) = delete;
    Flash(Flash&& other) = delete;
    Flash& operator=(Flash& other) = delete;
    Flash& operator=(Flash&& other) = delete;
    void setup() const noexcept;

    Option<AuthToken> readAuthToken() const noexcept;
    void removeAuthToken() const noexcept;
    void writeAuthToken(const AuthToken & token) const noexcept;

    Option<PlantId> readPlantId() const noexcept;
    void removePlantId() const noexcept;
    void writePlantId(const PlantId & id) const noexcept;

    Option<struct WifiCredentials> readWifiConfig() const noexcept;
    void removeWifiConfig() const noexcept;
    void writeWifiConfig(const struct WifiCredentials & id) const noexcept;
};

#include <utils.hpp>
#ifndef IOP_FLASH
  #define IOP_FLASH_DISABLED
#endif
// If we aren't online we will endup writing/removing dummy values, so let's not waste writes
#ifndef IOP_ONLINE
  #define IOP_FLASH_DISABLED
#endif
#ifndef IOP_MONITOR
  #define IOP_FLASH_DISABLED
#endif

#endif