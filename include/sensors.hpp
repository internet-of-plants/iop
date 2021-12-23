#ifndef IOP_SENSORS_HPP
#define IOP_SENSORS_HPP

#include "driver/sensors.hpp"
#include "utils.hpp"

/// Abstracts away sensors access, providing a cohesive state.
/// It's completely synchronous.
class Sensors {
#ifdef IOP_SENSORS
  driver::io::Pin soilResistivityPower;
  OneWire * soilTemperatureOneWireBus;
  DallasTemperature * soilTemperatureSensor;
  DHT * airTempAndHumiditySensor;
#endif

public:
  /// Initializes all sensors
  void setup() noexcept;

  /// Collects a monitoring event from the sensors
  auto measure() noexcept -> Event;

  Sensors(const driver::io::Pin soilResistivityPower,
          const driver::io::Pin soilTemperature, const driver::io::Pin dht,
          const uint8_t dhtVersion) noexcept;
  ~Sensors() noexcept;

  Sensors(Sensors const &other) noexcept = delete;
  Sensors(Sensors &&other) noexcept;
  auto operator=(Sensors const &other) noexcept -> Sensors & = delete;
  auto operator=(Sensors &&other) noexcept -> Sensors &;
};

#endif