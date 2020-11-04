#ifndef IOP_UTILS_H_
#define IOP_UTILS_H_

#include <Arduino.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <models.hpp>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle sensors dependency
#define IOP_SENSORS

enum LogLevel {
  TRACE = 0,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  CRIT
};

enum InterruptEvent {
  NONE,
  WPS
};

void handleInterrupt();
AuthToken stringToAuthToken(const String &val);
PlantId stringToPlantId(const String &val);
void measureAndSend(DHT &airTempAndHumidity, DallasTemperature &soilTemperature, const uint8_t soilResistivityPowerPin);

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

#undef panic
#define panic(msg) panic__(msg, __FILE__, __LINE__, __func__)
void panic__(const String msg, const String file, const int line, const String func) __attribute__((noreturn));

template<typename T>
class Option {
 private:
  bool filled;
  T value;
 public:
  Option(): filled(false), value({0}) {}

  Option(const T v): filled(true), value(v) {}
  bool isSome() const { return filled; }
  bool isNone() const { return !filled; }
  T unwrap() const { return expect("Tried to unwrap an empty Option"); }
  T expect(const String msg) const {
    if (!filled) { panic(msg); }
    return value;
  }
  T unwrap_or(const T or_) const {
    if (isSome()) {
      return value;
    }
    return or_;
  }
    
  template <typename U>
  Option<U> andThen(std::function<Option<U> (const T&)> f) const {
    if (isSome()) {
      return f(value);
    }
    return Option<U>();
  }

  template <typename U>
  Option<U> map(std::function<U (const T&)> f) const {
    if (isSome()) {
      return Option<U>(f(value));
    }
    return Option<U>();
  }
};

class Log {
 public:
  void trace(const String msg);
  void debug(const String msg);
  void info(const String msg);
  void warn(const String msg);
  void error(const String msg);
  void crit(const String msg);
  void log(const LogLevel level, const String msg);
};

static volatile enum InterruptEvent interruptEvent = NONE;

#endif