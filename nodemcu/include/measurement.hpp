#ifndef IOP_MEASUREMENT_H_
#define IOP_MEASUREMENT_H_

#include <DallasTemperature.h>
#include <DHT.h>
#include <cstdint>

namespace measurement {
float soilTemperatureCelsius(DallasTemperature &sensor);
float airTemperatureCelsius(DHT &dht);
float airHumidityPercentage(DHT &dht);
float airHeatIndexCelsius(DHT &dht);
uint16_t soilResistivityRaw(const uint8_t powerPin);
}

#include <utils.hpp>
#ifndef IOP_SERIAL
  #define IOP_MEASUREMENT_DISABLED
#endif

#endif