#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "driver/sensors.hpp"
#include "utils.hpp"

/// Abstracts away sensors access, providing a cohesive state.
/// It's completely synchronous.
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

  /// Initializes all sensors
  void setup() noexcept;

  /// Collects a monitoring event from the sensors
  auto measure() noexcept -> Event;

  Sensors(const gpio::Pin soilResistivityPower,
          const gpio::Pin soilTemperature, const gpio::Pin dht,
          const uint8_t dhtVersion) noexcept
#ifdef IOP_SENSORS
      : soilResistivityPower(soilResistivityPower),
        soilTemperatureSensor(),
        airTempAndHumiditySensor(static_cast<uint8_t>(dht), dhtVersion) {
    static OneWire oneWire(static_cast<uint8_t>(soilTemperature));
    this->soilTemperatureSensor = DallasTemperature(&oneWire);
#else
{
  (void) soilResistivityPower;
  (void) soilTemperature;
  (void) dht;
  (void) dhtVersion;
#endif
  }

  Sensors(Sensors const &other) = default;
  Sensors(Sensors &&other) = delete;
  auto operator=(Sensors const &other) -> Sensors & = default;
  auto operator=(Sensors &&other) -> Sensors & = delete;
};

#endif