#include <measurement.hpp>

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