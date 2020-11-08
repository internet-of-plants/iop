#include <sensors.hpp>
#include <measurement.hpp>

#include <Arduino.h>
#include <memory>

void Sensors::setup() {
  #ifdef IOP_SENSORS
    pinMode(this->soilResistivityPowerPin, OUTPUT);
    this->airTempAndHumiditySensor.begin();
    this->soilTemperatureSensor.begin();
  #endif
}

Event Sensors::measure(const PlantId plantId) {
  return (Event) {
    .airTemperatureCelsius = measureAirTemperatureCelsius(this->airTempAndHumiditySensor),
    .airHumidityPercentage = measureAirHumidityPercentage(this->airTempAndHumiditySensor),
    .airHeatIndexCelsius = measureAirHeatIndexCelsius(this->airTempAndHumiditySensor),
    .soilResistivityRaw = measureSoilResistivityRaw(this->soilResistivityPowerPin),
    .soilTemperatureCelsius = measureSoilTemperatureCelsius(this->soilTemperatureSensor),
    .plantId = plantId,
  };
}