#ifndef IOP_DRIVER_SENSORS_HPP
#define IOP_DRIVER_SENSORS_HPP

#include "driver/gpio.hpp"

class DHT;
class DallasTemperature;
class OneWire;

/*
#ifdef IOP_DESKTOP
struct OneWire {
  OneWire(uint8_t pin) { (void) pin; }
};
struct DallasTemperature {
  DallasTemperature(OneWire *ptr) { (void) ptr; }
};
struct DHT {
  DHT(uint8_t pin, uint8_t version) {
    (void) pin;
    (void) version;
  }
};
#else
#endif
*/

//#include <DHT.h>
//#include <DallasTemperature.h>
//#include <OneWire.h>
/*
#undef OUTPUT
#undef INPUT
#undef HIGH
#undef LOW
#undef RISING
#undef FALLING
#undef CHANGED
#undef LED_BUILTIN
#endif
*/

#endif