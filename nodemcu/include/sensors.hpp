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
    // Self-reference, so we need a pointer to pin it
    DallasTemperature soilTemperatureSensor;
    DHT airTempAndHumiditySensor;

  public:
    Sensors(const uint8_t soilResistivityPowerPin, const uint8_t soilTemperaturePin, const uint8_t dhtPin, const uint8_t dhtVersion) noexcept:
      soilResistivityPowerPin(soilResistivityPowerPin),
      soilTemperatureOneWireBus(new OneWire(soilTemperaturePin)),
      soilTemperatureSensor(soilTemperatureOneWireBus.get()),
      airTempAndHumiditySensor(dhtPin, dhtVersion) {}
    Sensors(Sensors& other) = delete;
    Sensors(Sensors&& other) = delete;
    Sensors& operator=(Sensors& other) = delete;
    Sensors& operator=(Sensors&& other) = delete;
    void setup() noexcept;
    Event measure(PlantId plantId) noexcept;
};

#include <utils.hpp>
#ifndef IOP_SENSORS
  #define IOP_SENSORS_DISABLED
#endif

#endif