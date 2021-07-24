#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "utils.hpp"
#include <memory>
#include "driver/sensors.hpp"

/// Abstracts away sensors access, providing a cohesive state. It's
/// completely synchronous.
class Sensors {
#ifdef IOP_SENSORS
  gpio::Pin soilResistivityPower;
  // We use a shared_ptr because we have a self-reference to this and
  // DallasTemperature only copies itself. So moving would mean
  // 'soilTemperatureSensor' points to a unique_ptr that it doesn't own, so
  // only copies are allowed. This is the simplest way. UNSAFE: SELF_REF
  //std::shared_ptr<OneWire> soilTemperatureOneWireBus;
  DallasTemperature soilTemperatureSensor;
  DHT airTempAndHumiditySensor;
#endif

public:
  Sensors(const gpio::Pin soilResistivityPower,
          const gpio::Pin soilTemperature, const gpio::Pin dht,
          const uint8_t dhtVersion) noexcept
#ifdef IOP_SENSORS
      : soilResistivityPower(soilResistivityPower),
        soilTemperatureSensor(),
        airTempAndHumiditySensor(static_cast<uint8_t>(dht), dhtVersion) {
    IOP_TRACE();
    static OneWire oneWire(static_cast<uint8_t>(soilTemperature));
    this->soilTemperatureSensor = DallasTemperature(&oneWire);
#else
{
  IOP_TRACE();
  (void) soilResistivityPower;
  (void) soilTemperature;
  (void) dht;
  (void) dhtVersion;
#endif
  }
  void setup() noexcept;
  auto measure() noexcept -> Event;
  ~Sensors() noexcept { IOP_TRACE(); }

  Sensors(Sensors const &other)
#ifdef IOP_SENSORS
      : soilResistivityPower(other.soilResistivityPower),
        soilTemperatureOneWireBus(other.soilTemperatureOneWireBus),
        soilTemperatureSensor(other.soilTemperatureSensor),
        airTempAndHumiditySensor(other.airTempAndHumiditySensor)
#endif
  {
    IOP_TRACE();
    (void) other;
  }
  Sensors(Sensors &&other) = delete;
  auto operator=(Sensors const &other) -> Sensors & {
    IOP_TRACE();
    if (this == &other)
      return *this;
#ifdef IOP_SENSORS
    this->soilResistivityPower = other.soilResistivityPower;
    this->soilTemperatureOneWireBus = other.soilTemperatureOneWireBus;
    this->soilTemperatureSensor = other.soilTemperatureSensor;
    this->airTempAndHumiditySensor = other.airTempAndHumiditySensor;
#endif
    return *this;
  }
  auto operator=(Sensors &&other) -> Sensors & = delete;
};

#endif