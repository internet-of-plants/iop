#include <sensors.hpp>

#ifndef IOP_SENSORS_DISABLED
#include <measurement.hpp>

void Sensors::setup() {
  pinMode(this->soilResistivityPowerPin, OUTPUT);
  this->airTempAndHumiditySensor.begin();
  this->soilTemperatureSensor.begin();
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
#endif

#ifdef IOP_SENSORS_DISABLED
void Sensors::setup() {}
Event Sensors::measure(const PlantId plantId) {
  Event ev = {0};
  ev.plantId = plantId;
  return ev;
}
#endif