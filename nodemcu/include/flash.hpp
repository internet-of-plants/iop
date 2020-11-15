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
    Flash(const LogLevel logLevel): logger(logLevel, F("FLASH")) {}
    Flash(Flash& other) = delete;
    Flash(Flash&& other) = delete;
    void operator=(Flash& other) = delete;
    void operator=(Flash&& other) = delete;
    void setup() const;

    Option<AuthToken> readAuthToken() const;
    void removeAuthToken() const;
    void writeAuthToken(const AuthToken & token) const;

    Option<PlantId> readPlantId() const;
    void removePlantId() const;
    void writePlantId(const PlantId & id) const;

    Option<struct WifiCredentials> readWifiConfig() const;
    void removeWifiConfig() const;
    void writeWifiConfig(const struct WifiCredentials & id) const;
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