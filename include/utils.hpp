#ifndef IOP_UTILS_HPP
#define IOP_UTILS_HPP

#include "core/tracer.hpp"
#include "core/utils.hpp"

#include <cstdint>
#include <memory>

// (Un)Comment this line to toggle credentials server dependency
#define IOP_SERVER

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle network logging
// TODO: we should make network logging into another task in cont.h, yielding to
// it when needed. Otherwise the stack will go cray cray
//#ifdef IOP_DESKTOP
#define IOP_NETWORK_LOGGING
//#endif

// (Un)Comment this line to toggle sensors dependency
// #define IOP_SENSORS

// (Un)Comment this line to toggle flash memory dependency
#define IOP_FLASH

// (Un)Comment this line to toggle factory reset dependency
#define IOP_FACTORY_RESET

// (Un)Comment this line to toggle over the air updates (OTA) dependency
#define IOP_OTA

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally
// and pretend the request didn't fail
// If IOP_MONITOR is defined, then it doesn't matter whether IOP_MOCK_MONITOR is
// defined
#define IOP_MOCK_MONITOR

#ifndef IOP_SERIAL
#ifdef IOP_NETWORK_LOGGING
#undef IOP_NETWORK_LOGGING
#endif
#endif

// If you change the number of interrupt types, please update interruptVariant
// to the correct size
enum class InterruptEvent { NONE, FACTORY_RESET, ON_CONNECTION, MUST_UPGRADE };
constexpr static const uint8_t interruptVariants = 4;

namespace utils {
void scheduleInterrupt(InterruptEvent ev) noexcept;
auto descheduleInterrupt() noexcept -> InterruptEvent;
std::string base64Encode(const uint8_t *in, const size_t size);
} // namespace utils

#endif