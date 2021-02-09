#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "models.hpp"
#include "utils.hpp"

#include "DHT.h"
#include "DallasTemperature.h"
#include "OneWire.h"

#include <memory>

/// Abstracts away sensors access, providing a cohesive state. It's
/// completely synchronous. TODO: allow async reads
class Sensors {
private:
  uint8_t soilResistivityPowerPin;
  // We use a shared_ptr because we have a self-reference to this and
  // DallasTemperature only copies itself. So moving would mean
  // 'soilTemperatureSensor' points to a unique_ptr that it doesn't own, so
  // only copies are allowed. This is the simplest way. UNSAFE: SELF_REF
  std::shared_ptr<OneWire> soilTemperatureOneWireBus;
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;

public:
  Sensors(const uint8_t soilResistivityPowerPin,
          const uint8_t soilTemperaturePin, const uint8_t dhtPin,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPowerPin(soilResistivityPowerPin),
        soilTemperatureOneWireBus(
            iop::try_make_shared<OneWire>(soilTemperaturePin)),
        /* SELF_REF: this is dangerous, although allocation helps a lot. */
        soilTemperatureSensor(soilTemperatureOneWireBus.get()),
        airTempAndHumiditySensor(dhtPin, dhtVersion) {
    IOP_TRACE();
    if (!this->soilTemperatureOneWireBus)
      iop_panic(F("Unable to allocate one wire bus for soil DallasTemperature"));
  }
  void setup() noexcept;
  auto measure() noexcept -> Event;
  ~Sensors() noexcept { IOP_TRACE(); }

  Sensors(Sensors const &other)
      : soilResistivityPowerPin(other.soilResistivityPowerPin),
        soilTemperatureOneWireBus(other.soilTemperatureOneWireBus),
        soilTemperatureSensor(other.soilTemperatureSensor),
        airTempAndHumiditySensor(other.airTempAndHumiditySensor) {
    IOP_TRACE();
  }
  Sensors(Sensors &&other) = delete;
  auto operator=(Sensors const &other) -> Sensors & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->soilResistivityPowerPin = other.soilResistivityPowerPin;
    this->soilTemperatureOneWireBus = other.soilTemperatureOneWireBus;
    this->soilTemperatureSensor = other.soilTemperatureSensor;
    this->airTempAndHumiditySensor = other.airTempAndHumiditySensor;
    return *this;
  }
  auto operator=(Sensors &&other) -> Sensors & = delete;
};

#include "utils.hpp"
#ifndef IOP_SENSORS
#define IOP_SENSORS_DISABLED
#endif

#endif