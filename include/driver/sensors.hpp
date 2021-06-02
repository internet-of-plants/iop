#ifndef IOP_DRIVER_SENSORS_HPP
#define IOP_DRIVER_SENSORS_HPP

#ifdef IOP_DESKTOP
class OneWire {
  uint8_t dummy;
public:
  OneWire(uint8_t pin) {
    (void) pin;
  }
};
class DallasTemperature {
  uint8_t dummy;
public:
  DallasTemperature(OneWire *ptr) {
    (void) ptr;
  }
};
class DHT {
  uint8_t dummy;
public:
  DHT(uint8_t pin, uint8_t version) {
    (void) pin;
    (void) version;
  }
};
#else
#include "DHT.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#endif

#endif