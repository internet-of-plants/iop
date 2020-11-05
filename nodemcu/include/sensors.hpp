#ifndef IOP_SENSORS_H_
#define IOP_SENSORS_H_

#include <memory>
#include <configuration.h>
#include <models.hpp>

#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>

class Sensors {
  private:
    std::unique_ptr<OneWire> soilTemperatureOneWireBus;
    DallasTemperature soilTemperatureSensor;
    DHT airTempAndHumiditySensor;

  public:
    Sensors();
    void setup();
    Event measure(const PlantId plantId);
};

#endif