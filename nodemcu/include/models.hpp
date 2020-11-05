#ifndef IOP_MODELS_H_
#define IOP_MODELS_H_

#include <Arduino.h>
#include <cstdint>

// TODO: Add type safety, so we can't conflict with arrays of the same size
// This will mea we can overload functions to have the same name (flash and api)
// and only depend on the type passed to it
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
