#ifndef IOP_MEASUREMENT_H
#define IOP_MEASUREMENT_H

#include "DHT.h"
#include "DallasTemperature.h"
#include <cstdint>

namespace measurement {
float soilTemperatureCelsius(DallasTemperature &sensor) noexcept;
float airTemperatureCelsius(DHT &dht) noexcept;
float airHumidityPercentage(DHT &dht) noexcept;
float airHeatIndexCelsius(DHT &dht) noexcept;
uint16_t soilResistivityRaw(const uint8_t powerPin) noexcept;
} // namespace measurement

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_MEASUREMENT_DISABLED
#endif

#endif