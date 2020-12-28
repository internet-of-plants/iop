#include <Arduino.h>
#include <measurement.hpp>

namespace measurement {
#ifndef IOP_MEASUREMENT_DISABLED
float soilTemperatureCelsius(DallasTemperature &sensor) noexcept {
  // Blocks until reading is done
  sensor.requestTemperatures();
  // Accessing by index is bad. It's slow, we should store the sensor's address
  // and use it
  return sensor.getTempCByIndex(0);
}

float airTemperatureCelsius(DHT &dht) noexcept { return dht.readTemperature(); }

float airHumidityPercentage(DHT &dht) noexcept { return dht.readHumidity(); }

float airHeatIndexCelsius(DHT &dht) noexcept { return dht.computeHeatIndex(); }

uint16_t soilResistivityRaw(const uint8_t powerPin) noexcept {
  digitalWrite(powerPin, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  uint16_t value1 = analogRead(A0);
  delay(500);
  uint16_t value2 = analogRead(A0);
  delay(500);
  uint16_t value = (value1 + value2 + analogRead(A0)) / 3;
  digitalWrite(powerPin, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  return value;
}
#endif

#ifdef IOP_MEASUREMENT_DISABLED
float soilTemperatureCelsius(DallasTemperature &sensor) noexcept { return 0.; }
float airTemperatureCelsius(DHT &dht) noexcept { return 0.; }
float airHumidityPercentage(DHT &dht) noexcept { return 0.; }
float airHeatIndexCelsius(DHT &dht) noexcept { return 0.; }
uint16_t soilResistivityRaw(const uint8_t powerPin) noexcept { return 0.; }
#endif
} // namespace measurement