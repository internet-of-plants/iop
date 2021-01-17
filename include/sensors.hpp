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
  OneWire soilTemperatureOneWireBus;
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;

public:
  ~Sensors() = default;
  Sensors(const uint8_t soilResistivityPowerPin,
          const uint8_t soilTemperaturePin, const uint8_t dhtPin,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPowerPin(soilResistivityPowerPin),
        soilTemperatureOneWireBus(soilTemperaturePin),
        // Self reference, this is dangerous. UNSAFE: SELF_REF
        soilTemperatureSensor(&soilTemperatureOneWireBus),
        airTempAndHumiditySensor(dhtPin, dhtVersion) {}
  void setup() noexcept;
  auto measure(PlantId plantId, MD5Hash firmwareHash) noexcept -> Event;

  // Self-referential class, it must not be moved or copied. SELF_REF
  Sensors(Sensors &other) = delete;
  Sensors(Sensors &&other) = delete;
  auto operator=(Sensors &other) -> Sensors & = delete;
  auto operator=(Sensors &&other) -> Sensors & = delete;
};

#include "utils.hpp"
#ifndef IOP_SENSORS
#define IOP_SENSORS_DISABLED
#endif

#endif