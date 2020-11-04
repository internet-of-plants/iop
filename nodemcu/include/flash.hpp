#ifndef IOP_FLASH_H_
#define IOP_FLASH_H_

#include <utils.hpp>
#include <ESP8266WiFi.h>

Option<AuthToken> readAuthTokenFromEEPROM();
void removeAuthTokenFromEEPROM();
void writeAuthTokenToEEPROM(const AuthToken token);

// i64 as string has at most 19 characters
Option<PlantId> readPlantIdFromEEPROM();
void removePlantIdFromEEPROM();
void writePlantIdToEEPROM(const PlantId token);

Option<struct station_config> readWifiConfigFromEEPROM();
void removeWifiConfigFromEEPROM();
void writeWifiConfigToEEPROM(const struct station_config config);

#endif