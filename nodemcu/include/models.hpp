#ifndef IOP_MODELS_H_
#define IOP_MODELS_H_

#include <Arduino.h>
#include <cstdint>

typedef std::array<uint8_t, 64> AuthToken;
typedef std::array<uint8_t, 19> PlantId;

typedef struct event {
  float airTemperatureCelsius;
  float airHumidityPercentage;
  float airHeatIndexCelsius;
  uint16_t soilResistivityRaw;
  float soilTemperatureCelsius;
  String plantId;
} Event;

#endif
