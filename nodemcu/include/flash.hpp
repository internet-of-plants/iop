#ifndef IOP_FLASH_H_
#define IOP_FLASH_H_

#include <utils.hpp>
#include <ESP8266WiFi.h>

class Flash {
  public:
    Flash() {}
    Flash(const Flash& obj) = delete;
    Flash& operator=(const Flash& obj) = delete;

    void setup() const;

    Option<AuthToken> readAuthToken() const;
    void removeAuthToken() const;
    void writeAuthToken(const AuthToken token) const;

    Option<PlantId> readPlantId() const;
    void removePlantId() const;
    void writePlantId(const PlantId id) const;

    Option<struct station_config> readWifiConfig() const;
    void removeWifiConfig() const;
    void writeWifiConfig(const struct station_config id) const;
};

const static Flash flash;

#endif