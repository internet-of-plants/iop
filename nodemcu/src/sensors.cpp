#include <sensors.hpp>
#include <measurement.hpp>
#include <flash.hpp>

#include <Arduino.h>
#include <memory>

Sensors::Sensors(): airTempAndHumiditySensor(DHT(airTempAndHumidityPin, dhtVersion)) {
  soilTemperatureOneWireBus = std::unique_ptr<OneWire>(new OneWire(soilTemperaturePin));
  soilTemperatureSensor = DallasTemperature(soilTemperatureOneWireBus.get());
}

void Sensors::setup() {
  #ifdef IOP_SENSORS
    pinMode(soilResistivityPowerPin, OUTPUT);
    airTempAndHumiditySensor.begin();
    soilTemperatureSensor.begin();
  #endif
}

Event Sensors::measure(const PlantId plantId) {
  return (Event) {
    .airTemperatureCelsius = measureAirTemperatureCelsius(airTempAndHumiditySensor),
    .airHumidityPercentage = measureAirHumidityPercentage(airTempAndHumiditySensor),
    .airHeatIndexCelsius = measureAirHeatIndexCelsius(airTempAndHumiditySensor),
    .soilResistivityRaw = measureSoilResistivityRaw(soilResistivityPowerPin),
    .soilTemperatureCelsius = measureSoilTemperatureCelsius(soilTemperatureSensor),
    .plantId = (char*) plantId.data(),
  };
}