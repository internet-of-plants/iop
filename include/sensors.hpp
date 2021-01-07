#ifndef IOP_SENSORS_H
#define IOP_SENSORS_H

#include "models.hpp"
#include <memory>

#include "DHT.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "utils.hpp"

/// Abstracts away sensors access, providing a cohesive state. It's completely
/// synchronous.
class Sensors {
private:
  uint8_t soilResistivityPowerPin;
  std::unique_ptr<OneWire> soilTemperatureOneWireBus;
  // Self-reference, so we need a pointer to pin it
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;

public:
  Sensors(const uint8_t soilResistivityPowerPin,
          const uint8_t soilTemperaturePin, const uint8_t dhtPin,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPowerPin(soilResistivityPowerPin),
        soilTemperatureOneWireBus(make_unique<OneWire>(soilTemperaturePin)),
        soilTemperatureSensor(soilTemperatureOneWireBus.get()),
        airTempAndHumiditySensor(dhtPin, dhtVersion) {}
  Sensors(Sensors &other) = delete;
  Sensors(Sensors &&other) = delete;
  Sensors &operator=(Sensors &other) = delete;
  Sensors &operator=(Sensors &&other) = delete;
  void setup() noexcept;
  Event measure(PlantId plantId, MD5Hash firmwareHash) noexcept;
};

#include "utils.hpp"
#ifndef IOP_SENSORS
#define IOP_SENSORS_DISABLED
#endif

#endif