#include "sensors.hpp"
#include "utils.hpp"

#ifdef IOP_SENSORS
void Sensors::setup() noexcept {
  IOP_TRACE();
  pinMode(this->soilResistivityPowerPin, OUTPUT);
  this->airTempAndHumiditySensor.begin();
  this->soilTemperatureSensor.begin();
}

/// Handles low level access to hardware connected devices
namespace measurement {
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float;
auto airTemperatureCelsius(DHT &dht) noexcept -> float;
auto airHumidityPercentage(DHT &dht) noexcept -> float;
auto airHeatIndexCelsius(DHT &dht) noexcept -> float;
auto soilResistivityRaw(uint8_t powerPin) noexcept -> uint16_t;
} // namespace measurement

auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
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
      });
}

namespace measurement {
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float {
  IOP_TRACE();
  // Blocks until reading is done
  sensor.requestTemperatures();
  // Accessing by index is bad. It's slow, we should store the sensor's address
  // and use it
  return sensor.getTempCByIndex(0);
}

auto airTemperatureCelsius(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.readTemperature();
}

auto airHumidityPercentage(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.readHumidity();
}

auto airHeatIndexCelsius(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.computeHeatIndex();
}

auto soilResistivityRaw(const uint8_t powerPin) noexcept -> uint16_t {
  IOP_TRACE();
  digitalWrite(powerPin, HIGH);
  delay(2000); // NOLINT *-avoid-magic-numbers
  uint16_t value1 = analogRead(A0);
  delay(500); // NOLINT *-avoid-magic-numbers
  uint16_t value2 = analogRead(A0);
  delay(500); // NOLINT *-avoid-magic-numbers
  uint16_t value = (value1 + value2 + analogRead(A0)) / 3;
  digitalWrite(powerPin, LOW);
  return value;
}
} // namespace measurement
#else
void Sensors::setup() noexcept {
  IOP_TRACE();
  (void)*this;
}
auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
  (void)*this;
  return Event((EventStorage){0});
}
#endif