#ifndef IOP_UTILS_H_
#define IOP_UTILS_H_

#include <Arduino.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <models.hpp>
#include <option.hpp>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle sensors dependency
//#define IOP_SENSORS

enum InterruptEvent {
  NONE,
  FACTORY_RESET
};

String clone(const String& value);
uint64_t hashString(const String string);
bool handlePlantId(const AuthToken token, const String macAddress);

class MockSerial {
  public:
    MockSerial() {}
    void begin(unsigned long baud) {}
    void flush() {}
    size_t println(const char * msg) { return 0; }
    size_t println(const String &s) { return 0; }
};

static MockSerial mockSerial;

#ifndef IOP_SERIAL
  #define Serial MockSerial
#endif

static volatile enum InterruptEvent interruptEvent = NONE;

#endif