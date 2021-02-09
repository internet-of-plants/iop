#ifndef IOP_UTILS_HPP
#define IOP_UTILS_HPP

#include "Arduino.h"
#include "core/tracer.hpp"

#include <cstdint>
#include <memory>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle credentials server dependency
#define IOP_SERVER

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle network logging
// TODO: we should make network logging into another task in cont.h, yielding to
// it when needed. Otherwise the stack will go cray cray
// #define IOP_NETWORK_LOGGING

// (Un)Comment this line to toggle sensors dependency
// #define IOP_SENSORS

// (Un)Comment this line to toggle flash memory dependency
#define IOP_FLASH

// (Un)Comment this line to toggle factory reset dependency
#define IOP_FACTORY_RESET

// (Un)Comment this line to toggle over the air updates (OTA) dependency
#define IOP_OTA

// (Un)Comment this line to toggle memory stats logging
// TODO: log memory to server, storing the min and max for the interval
// #define LOG_MEMORY

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally
// and pretend the request didn't fail
// If IOP_MONITOR is defined, then it doesn't matter whether IOP_MOCK_MONITOR is
// defined
#define IOP_MOCK_MONITOR

using esp_time = unsigned long; // NOLINT google-runtime-int

// If you change the number of interrupt types, please update interruptVariant
// to the correct size
enum class InterruptEvent { NONE, FACTORY_RESET, ON_CONNECTION, MUST_UPGRADE };
constexpr static const uint8_t interruptVariants = 4;

class MD5Hash_class;
class MacAddress_class;
class Log;

namespace iop {
class StringView;
class CowString;
} // namespace iop

// The `_class` suffix is just to make forward declaration works with macro
// created classes. The actual class has that suffix, but a `using Type =
// Type_class;` is set. But it's not here and if done here it will conflict.

namespace utils {
auto hashSketch() noexcept -> const MD5Hash_class &;
auto macAddress() noexcept -> const MacAddress_class &;
void ICACHE_RAM_ATTR scheduleInterrupt(InterruptEvent ev) noexcept;
auto ICACHE_RAM_ATTR descheduleInterrupt() noexcept -> InterruptEvent;
void logMemory(const Log &logger) noexcept;
} // namespace utils

#endif