#ifndef MODELS_H
#define MODELS_H_

#include <Arduino.h>
#include <cstdint>

typedef struct event {
  uint8_t airTemperatureCelsius;
  uint8_t airHumidityPercentage;
  uint16_t soilResistivityRaw;
  uint8_t soilTemperatureCelsius;
  String plantId;
} Event;

#endif
