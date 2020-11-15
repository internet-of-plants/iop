#include <sensors.hpp>

#ifndef IOP_SENSORS_DISABLED
#include <measurement.hpp>

void Sensors::setup() {
  pinMode(this->soilResistivityPowerPin, OUTPUT);
  this->airTempAndHumiditySensor.begin();
  this->soilTemperatureSensor.begin();
}

Event Sensors::measure(PlantId plantId) {
  return Event((EventStorage) {
    .airTemperatureCelsius = measurement::airTemperatureCelsius(this->airTempAndHumiditySensor),
    .airHumidityPercentage = measurement::airHumidityPercentage(this->airTempAndHumiditySensor),
    .airHeatIndexCelsius = measurement::airHeatIndexCelsius(this->airTempAndHumiditySensor),
    .soilResistivityRaw = measurement::soilResistivityRaw(this->soilResistivityPowerPin),
    .soilTemperatureCelsius = measurement::soilTemperatureCelsius(this->soilTemperatureSensor),
  }, std::move(plantId));
}
#endif

#ifdef IOP_SENSORS_DISABLED
void Sensors::setup() {}
Event Sensors::measure(PlantId plantId) {
  return Event((EventStorage) {0}, std::move(plantId));
}
#endif