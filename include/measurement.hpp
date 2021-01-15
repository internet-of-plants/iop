#ifndef IOP_MEASUREMENT_HPP
#define IOP_MEASUREMENT_HPP

#include "DHT.h"
#include "DallasTemperature.h"
#include <cstdint>

namespace measurement {
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float;
auto airTemperatureCelsius(DHT &dht) noexcept -> float;
auto airHumidityPercentage(DHT &dht) noexcept -> float;
auto airHeatIndexCelsius(DHT &dht) noexcept -> float;
auto soilResistivityRaw(uint8_t powerPin) noexcept -> uint16_t;
} // namespace measurement

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_MEASUREMENT_DISABLED
#endif

#endif