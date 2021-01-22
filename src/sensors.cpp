#include "sensors.hpp"

#ifndef IOP_SENSORS_DISABLED
#include "measurement.hpp"

void Sensors::setup() noexcept {
  pinMode(this->soilResistivityPowerPin, OUTPUT);
  this->airTempAndHumiditySensor.begin();
  this->soilTemperatureSensor.begin();
}

auto Sensors::measure(MacAddress mac, MD5Hash firmwareHash) noexcept -> Event {
  return Event(
      (EventStorage){
          .airTemperatureCelsius = measurement::airTemperatureCelsius(
              this->airTempAndHumiditySensor),
          .airHumidityPercentage = measurement::airHumidityPercentage(
              this->airTempAndHumiditySensor),
          .airHeatIndexCelsius =
              measurement::airHeatIndexCelsius(this->airTempAndHumiditySensor),
          .soilResistivityRaw =
              measurement::soilResistivityRaw(this->soilResistivityPowerPin),
          .soilTemperatureCelsius =
              measurement::soilTemperatureCelsius(this->soilTemperatureSensor),
      },
      std::move(mac), std::move(firmwareHash));
}
#else
void Sensors::setup() noexcept {}
Event Sensors::measure(MacAddress macAddress, MD5Hash firmwareHash) noexcept {
  return Event((EventStorage){0}, std::move(macAddress),
               std::move(firmwareHash));
}
#endif