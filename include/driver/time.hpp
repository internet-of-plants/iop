#ifndef IOP_DRIVER_TIME
#define IOP_DRIVER_TIME

#include <chrono>
#include <thread>

static void delay(uint64_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
const auto start = std::chrono::system_clock::now().time_since_epoch();
static uint64_t millis() { return std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() - start).time_since_epoch()).count(); }

#endif