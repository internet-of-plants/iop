#ifndef IOP_UTILS_H_
#define IOP_UTILS_H_

// (Un)Comment this line to toggle server dependency
//#define IOP_ONLINE

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

#include <Arduino.h>

#undef panic

class MockSerial {
  public:
    MockSerial() {}
    void begin(unsigned long baud) {}
    void flush() {};
    size_t println(const char * msg) { return 0; }
    size_t println(const String &s) { return 0; }
};

static MockSerial mockSerial;

#ifndef IOP_SERIAL
  #define Serial MockSerial
#endif

void panic(const String msg) __attribute__((noreturn));

template<typename T>
class Option {
 private:
  bool filled;
  T value;
 public:
  Option(): filled(false) {}
  Option(const T value): filled(true), value(value) {}
  bool isSome() const { return filled; }
  bool isNone() const { return !filled; }

  T unwrap() const {
    return expect("Tried to unwrap None Option");
  }

  T expect(const String msg) const {
    if (!filled) {
      panic(msg);
    }
    return value;
  }
};


typedef struct response {
  int code;
  Option<String> payload;
} Response;

Option<Response> httpPost(const String token, const String path, const String data);
Option<Response> httpPost(const String path, const String data);
Option<Response> httpPost(const Option<String> token, const String path, const String data);
bool connect();

#endif