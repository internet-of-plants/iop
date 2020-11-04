#ifndef IOP_MEASUREMENT_H_
#define IOP_MEASUREMENT_H_

#include <DallasTemperature.h>
#include <DHT.h>

float measureSoilTemperatureCelsius(DallasTemperature &sensor);
float measureAirTemperatureCelsius(DHT &dht);
float measureAirHumidityPercentage(DHT &dht);
float measureAirHeatIndexCelsius(DHT &dht);
uint16_t measureSoilResistivityRaw(const uint8_t powerPin);

#endif