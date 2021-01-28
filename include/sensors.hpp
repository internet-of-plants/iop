#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "certificate_storage.hpp"

#include "models.hpp"

#include "DHT.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "utils.hpp"

#include <memory>

/// Abstracts away sensors access, providing a cohesive state. It's completely
/// synchronous.
class Sensors {
private:
  uint8_t soilResistivityPowerPin;
  // We use a shared_ptr because we have a self-reference to it and
  // DallasTemperature only copies itself. So moving would mean
  // 'soilTemperatureSensor' points to a unique_ptr that it doesn't own, so
  // only copies are allowed. UNSAFE: SELF_REF
  std::shared_ptr<OneWire> soilTemperatureOneWireBus;
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;

public:
  ~Sensors() { IOP_TRACE(); }
  Sensors(const uint8_t soilResistivityPowerPin,
          const uint8_t soilTemperaturePin, const uint8_t dhtPin,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPowerPin(soilResistivityPowerPin),
        soilTemperatureOneWireBus(try_make_shared<OneWire>(soilTemperaturePin)),
        // SELF_REF: this is dangerous, although allocation helps a lot.
        soilTemperatureSensor(soilTemperatureOneWireBus.get()),
        airTempAndHumiditySensor(dhtPin, dhtVersion) {
    IOP_TRACE();
    if (!this->soilTemperatureOneWireBus)
      panic_(F("Unable to allocate one wire bus for soil DallasTemperature"));
  }
  void setup() noexcept;
  auto measure(MacAddress mac, MD5Hash firmwareHash) noexcept -> Event;

  // Self-referential class, it must not be moved or copied. SELF_REF
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