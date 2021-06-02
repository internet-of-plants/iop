#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#ifdef IOP_DESKTOP
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "driver/time.hpp"

static void yield() {}

static void __panic_func(const char *file, uint16_t line, const char *func) noexcept __attribute__((noreturn));
void __panic_func(const char *file, uint16_t line, const char *func) noexcept {
  std::abort();
}
class Esp {
public:
  uint16_t getFreeHeap() { return 1000; }
  uint16_t getFreeContStack() { return 1000; }
  uint16_t getMaxFreeBlockSize() { return 1000; }
  uint16_t getHeapFragmentation() { return 0; }
  uint16_t getFreeSketchSpace() { return 1000; }
  void deepSleep(uint32_t microsecs) { 
    std::this_thread::sleep_for(std::chrono::microseconds(microsecs));
  }
  std::string getSketchMD5() { return "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"; }
};
static Esp ESP;
#define STATION_IF 0
static void wifi_get_macaddr(uint8_t station, uint8_t *buff) {
  (void) station;
  memcpy(buff, "AA:AA:AA:AA:AA:AA", 18);
}
#define sprintf_P sprintf
#else
#include "Esp.h"
#include "user_interface.h"
#endif

#endif