#include "measurement.hpp"
#include "Arduino.h"

namespace measurement {
#ifndef IOP_MEASUREMENT_DISABLED
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float {
  // Blocks until reading is done
  sensor.requestTemperatures();
  // Accessing by index is bad. It's slow, we should store the sensor's address
  // and use it
  return sensor.getTempCByIndex(0);
}

auto airTemperatureCelsius(DHT &dht) noexcept -> float {
  return dht.readTemperature();
}

auto airHumidityPercentage(DHT &dht) noexcept -> float {
  return dht.readHumidity();
}

auto airHeatIndexCelsius(DHT &dht) noexcept -> float {
  return dht.computeHeatIndex();
}

auto soilResistivityRaw(const uint8_t powerPin) noexcept -> uint16_t {
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
#endif

#ifdef IOP_MEASUREMENT_DISABLED
float soilTemperatureCelsius(DallasTemperature &sensor) noexcept { return 0.; }
float airTemperatureCelsius(DHT &dht) noexcept { return 0.; }
float airHumidityPercentage(DHT &dht) noexcept { return 0.; }
float airHeatIndexCelsius(DHT &dht) noexcept { return 0.; }
uint16_t soilResistivityRaw(const uint8_t powerPin) noexcept { return 0.; }
#endif
} // namespace measurement