#ifndef IOP_SENSORS_H_
#define IOP_SENSORS_H_

#include <memory>
#include <models.hpp>

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>

/// Abstracts away sensors access, providing a cohesive state. It's completely synchronous.
class Sensors {
  private:
    uint8_t soilResistivityPowerPin;
    std::unique_ptr<OneWire> soilTemperatureOneWireBus;
    DallasTemperature soilTemperatureSensor;
    DHT airTempAndHumiditySensor;

  public:
    Sensors(const uint8_t soilResistivityPowerPin, const uint8_t soilTemperaturePin, const uint8_t dhtPin, const uint8_t dhtVersion):
      soilResistivityPowerPin(soilResistivityPowerPin),
      soilTemperatureOneWireBus(new OneWire(soilTemperaturePin)),
      soilTemperatureSensor(soilTemperatureOneWireBus.get()),
      airTempAndHumiditySensor(dhtPin, dhtVersion) {}
    Sensors(Sensors& other) = delete;
    Sensors(Sensors&& other) = delete;
    void operator=(Sensors& other) = delete;
    void operator=(Sensors&& other) = delete;
    void setup();
    Event measure(PlantId plantId);
};

#include <utils.hpp>
#ifndef IOP_SENSORS
  #define IOP_SENSORS_DISABLED
#endif

#endif