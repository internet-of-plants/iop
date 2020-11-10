#ifndef IOP_UTILS_H_
#define IOP_UTILS_H_

#include <WString.h>
#include <models.hpp>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle credentials server dependency
//#define IOP_SERVER

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle sensors dependency
//#define IOP_SENSORS

// (Un)Comment this line to toggle flash memory dependency
//#define IOP_FLASH

// (Un)Comment this line to toggle factory reset dependency
//#define IOP_FACTORY_RESET

enum InterruptEvent {
  NONE,
  FACTORY_RESET,
  ON_CONNECTION
};

uint64_t hashString(const String string);

class MockSerial {
  public:
    MockSerial() {}
    void begin(unsigned long baud) {}
    void flush() {}
    size_t println(const char * msg) { return 0; }
    size_t println(const String &s) { return 0; }
};

static MockSerial mockSerial;

static volatile enum InterruptEvent interruptEvent = NONE;

#endif