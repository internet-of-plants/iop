#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "models.hpp"
#include "utils.hpp"
#include <memory>
#include "driver/sensors.hpp"

/// Abstracts away sensors access, providing a cohesive state. It's
/// completely synchronous.
class Sensors {
private:
  gpio::Pin soilResistivityPower;
  // We use a shared_ptr because we have a self-reference to this and
  // DallasTemperature only copies itself. So moving would mean
  // 'soilTemperatureSensor' points to a unique_ptr that it doesn't own, so
  // only copies are allowed. This is the simplest way. UNSAFE: SELF_REF
  std::shared_ptr<OneWire> soilTemperatureOneWireBus;
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;

public:
  Sensors(const gpio::Pin soilResistivityPower,
          const gpio::Pin soilTemperature, const gpio::Pin dht,
          const uint8_t dhtVersion) noexcept
      : soilResistivityPower(soilResistivityPower),
        soilTemperatureOneWireBus(
            iop::try_make_shared<OneWire>(static_cast<uint8_t>(soilTemperature))),
        /* SELF_REF: this is dangerous, although allocation helps a lot. */
        soilTemperatureSensor(soilTemperatureOneWireBus.get()),
        airTempAndHumiditySensor(static_cast<uint8_t>(dht), dhtVersion) {
    IOP_TRACE();
    if (!this->soilTemperatureOneWireBus)
      iop_panic(
          F("Unable to allocate one wire bus for soil DallasTemperature"));
  }
  void setup() noexcept;
  auto measure() noexcept -> Event;
  ~Sensors() noexcept { IOP_TRACE(); }

  Sensors(Sensors const &other)
      : soilResistivityPower(other.soilResistivityPower),
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

    this->soilResistivityPower = other.soilResistivityPower;
    this->soilTemperatureOneWireBus = other.soilTemperatureOneWireBus;
    this->soilTemperatureSensor = other.soilTemperatureSensor;
    this->airTempAndHumiditySensor = other.airTempAndHumiditySensor;
    return *this;
  }
  auto operator=(Sensors &&other) -> Sensors & = delete;
};

#endif