#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

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
  ~Sensors() = default;
  Sensors(const uint8_t soilResistivityPowerPin,
          const uint8_t soilTemperaturePin, const uint8_t dhtPin,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPowerPin(soilResistivityPowerPin),
        soilTemperatureOneWireBus(try_make_unique<OneWire>(soilTemperaturePin)),
        soilTemperatureSensor(soilTemperatureOneWireBus.get()),
        airTempAndHumiditySensor(dhtPin, dhtVersion) {
    if (!soilTemperatureOneWireBus)
      panic_(F("Unnable to allocate soilTemperatureOneWireBus"));
  }
  Sensors(Sensors &other) = delete;
  Sensors(Sensors &&other) = delete;
  auto operator=(Sensors &other) -> Sensors & = delete;
  auto operator=(Sensors &&other) -> Sensors & = delete;
  void setup() noexcept;
  auto measure(PlantId plantId, MD5Hash firmwareHash) noexcept -> Event;
};

#include "utils.hpp"
#ifndef IOP_SENSORS
#define IOP_SENSORS_DISABLED
#endif

#endif