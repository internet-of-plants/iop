#include "core/log.hpp"
#include "sensors.hpp"
#include "utils.hpp"

#ifdef IOP_SENSORS
#include "core/panic.hpp"

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#undef HIGH
#undef LOW
#undef OUTPUT

void Sensors::setup() noexcept {
  IOP_TRACE();
  gpio::gpio.mode(this->soilResistivityPower, gpio::Mode::OUTPUT);
  this->airTempAndHumiditySensor->begin();
  this->soilTemperatureSensor->begin();
}

/// Handles low level access to hardware connected devices
namespace measurement {
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float;
auto airTemperatureCelsius(DHT &dht) noexcept -> float;
auto airHumidityPercentage(DHT &dht) noexcept -> float;
auto airHeatIndexCelsius(DHT &dht) noexcept -> float;
auto soilResistivityRaw(gpio::Pin power) noexcept -> uint16_t;
} // namespace measurement

auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
  return (Event) {
    .airTemperatureCelsius = measurement::airTemperatureCelsius(*this->airTempAndHumiditySensor),
    .airHumidityPercentage = measurement::airHumidityPercentage(*this->airTempAndHumiditySensor),
    .airHeatIndexCelsius = measurement::airHeatIndexCelsius(*this->airTempAndHumiditySensor),
    .soilTemperatureCelsius = measurement::soilTemperatureCelsius(*this->soilTemperatureSensor),
    .soilResistivityRaw = measurement::soilResistivityRaw(this->soilResistivityPower),
  };
}

namespace measurement {
auto soilTemperatureCelsius(DallasTemperature &sensor) noexcept -> float {
  IOP_TRACE();
  // Blocks until reading is done
  sensor.requestTemperatures();
  // Accessing by index is bad. It's slow, we should store the sensor's address
  // and use it
  return sensor.getTempCByIndex(0);
}

auto airTemperatureCelsius(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.readTemperature();
}

auto airHumidityPercentage(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.readHumidity();
}

auto airHeatIndexCelsius(DHT &dht) noexcept -> float {
  IOP_TRACE();
  return dht.computeHeatIndex();
}

auto soilResistivityRaw(const gpio::Pin power) noexcept -> uint16_t {
  IOP_TRACE();
  digitalWrite(static_cast<uint8_t>(power), static_cast<uint8_t>(gpio::Data::HIGH));
  delay(2000); // NOLINT *-avoid-magic-numbers
  uint16_t value1 = analogRead(A0);
  delay(500); // NOLINT *-avoid-magic-numbers
  uint16_t value2 = analogRead(A0);
  delay(500); // NOLINT *-avoid-magic-numbers
  uint16_t value = (value1 + value2 + analogRead(A0)) / 3;
  digitalWrite(static_cast<uint8_t>(power), static_cast<uint8_t>(gpio::Data::LOW));
  return value;
}
} // namespace measurement
#else
void Sensors::setup() noexcept {
  IOP_TRACE();
  (void)*this;
}
auto Sensors::measure() noexcept -> Event {
  IOP_TRACE();
  (void)*this;
  return (Event) {
    .airTemperatureCelsius = 0.0,
    .airHumidityPercentage = 0.0,
    .airHeatIndexCelsius = 0.0,
    .soilTemperatureCelsius = 0.0,
    .soilResistivityRaw = 0,
  };
}

#endif

Sensors::~Sensors() noexcept {
#ifdef IOP_SENSORS
  delete this->airTempAndHumiditySensor;
  delete this->soilTemperatureSensor;
  delete this->soilTemperatureOneWireBus;
#endif
}

Sensors::Sensors(Sensors &&other) noexcept
#ifdef IOP_SENSORS
      : soilResistivityPower(other.soilResistivityPower), soilTemperatureOneWireBus(other.soilTemperatureOneWireBus),
        soilTemperatureSensor(other.soilTemperatureSensor), airTempAndHumiditySensor(other.airTempAndHumiditySensor) {
  other.soilTemperatureOneWireBus = nullptr;
  other.soilTemperatureSensor = nullptr;
  other.airTempAndHumiditySensor = nullptr;
}
#else
{}
#endif

auto Sensors::operator=(Sensors &&other) noexcept -> Sensors & {
#ifdef IOP_SENSORS
  this->soilResistivityPower = other.soilResistivityPower;
  this->soilTemperatureOneWireBus = other.soilTemperatureOneWireBus;
  this->soilTemperatureSensor = other.soilTemperatureSensor;
  this->airTempAndHumiditySensor = other.airTempAndHumiditySensor;

  other.soilTemperatureOneWireBus = nullptr;
  other.soilTemperatureSensor = nullptr;
  other.airTempAndHumiditySensor = nullptr;
#endif
  return *this;
}

Sensors::Sensors(const gpio::Pin soilResistivityPower,
          const gpio::Pin soilTemperature, const gpio::Pin dht,
          const uint8_t dhtVersion) noexcept
#ifdef IOP_SENSORS
      : soilResistivityPower(soilResistivityPower),
        soilTemperatureOneWireBus(new (std::nothrow) OneWire(static_cast<uint8_t>(soilTemperature))),
        soilTemperatureSensor(nullptr),
        airTempAndHumiditySensor(new (std::nothrow) DHT(static_cast<uint8_t>(dht), dhtVersion)) {
    iop_assert(this->airTempAndHumiditySensor, FLASH("Unable to allocate air temp and humidity sensor"));
    iop_assert(this->soilTemperatureOneWireBus, FLASH("Unable to allocate one wire bus"));
    this->soilTemperatureSensor = new (std::nothrow) DallasTemperature(this->soilTemperatureOneWireBus);
    iop_assert(this->soilTemperatureSensor, FLASH("Unable to allocate soil temperature sensor"));
}
#else
{
  (void) soilResistivityPower;
  (void) soilTemperature;
  (void) dht;
  (void) dhtVersion;
}
#endif