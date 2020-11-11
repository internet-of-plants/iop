#ifndef IOP_SENSORS_H_
#define IOP_SENSORS_H_

#include <memory>
#include <models.hpp>

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>

class Sensors {
  private:
    uint8_t soilResistivityPowerPin;
    std::unique_ptr<OneWire> soilTemperatureOneWireBus;
    DallasTemperature soilTemperatureSensor;
    DHT airTempAndHumiditySensor;

  public:
    Sensors(const uint8_t soilResistivityPowerPin, const uint8_t soilTemperaturePin, const uint8_t dhtPin, const uint8_t dhtVersion):
      soilResistivityPowerPin(soilResistivityPowerPin),
      soilTemperatureOneWireBus(new OneWire(soilTemperaturePin)),
      soilTemperatureSensor(soilTemperatureOneWireBus.get()),
      airTempAndHumiditySensor(dhtPin, dhtVersion) {}
    void operator=(Sensors& other) = delete;
    void operator=(Sensors&& other) {
      this->soilResistivityPowerPin = other.soilResistivityPowerPin;
      this->soilTemperatureOneWireBus = std::move(other.soilTemperatureOneWireBus);
      this->soilTemperatureSensor = std::move(other.soilTemperatureSensor);
      this->airTempAndHumiditySensor = std::move(other.airTempAndHumiditySensor);
    }
    Sensors(Sensors&& other):
      soilResistivityPowerPin(other.soilResistivityPowerPin),
      soilTemperatureOneWireBus(std::move(other.soilTemperatureOneWireBus)),
      soilTemperatureSensor(std::move(other.soilTemperatureSensor)),
      airTempAndHumiditySensor(std::move(other.airTempAndHumiditySensor)) {}

    void setup();
    Event measure(PlantId plantId);
};

#include <utils.hpp>
#ifndef IOP_SENSORS
  #define IOP_SENSORS_DISABLED
#endif

#endif