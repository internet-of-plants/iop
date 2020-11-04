#include <measurement.hpp>
#include <utils.hpp>

#ifdef IOP_SENSORS
float measureSoilTemperatureCelsius(DallasTemperature &sensor) {
  // Blocks until reading is done
  sensor.requestTemperatures();
  // Accessing by index is bad. It's slow, we should store the sensor's address and use it
  return sensor.getTempCByIndex(0);
}

float measureAirTemperatureCelsius(DHT &dht) {
  return dht.readTemperature();
}

float measureAirHumidityPercentage(DHT &dht) {
  return dht.readHumidity();
}

float measureAirHeatIndexCelsius(DHT &dht) {
  return dht.computeHeatIndex();
}

uint16_t measureSoilResistivityRaw(const uint8_t powerPin) {
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

#ifndef IOP_SENSORS
float measureSoilTemperatureCelsius(DallasTemperature &sensor) { return 0.; }
float measureAirTemperatureCelsius(DHT &dht) { return 0.; }
float measureAirHumidityPercentage(DHT &dht) { return 0.; }
float measureAirHeatIndexCelsius(DHT &dht) { return 0.; }
uint16_t measureSoilResistivityRaw(const uint8_t powerPin) { return 0.; }
#endif