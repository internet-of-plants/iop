#ifndef IOP_UTILS_H
#define IOP_UTILS_H

#include <cstdint>

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle credentials server dependency
#define IOP_SERVER

// (Un)Comment this line to toggle monitor server dependency
#define IOP_MONITOR

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle sensors dependency
#define IOP_SENSORS

// (Un)Comment this line to toggle flash memory dependency
#define IOP_FLASH

// (Un)Comment this line to toggle factory reset dependency
#define IOP_FACTORY_RESET

// (Un)Comment this line to toggle over the air updates (OTA) dependency
//#define IOP_OTA

// (Un)Comment this line to toggle memory stats logging
//#define LOG_MEMORY

// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally
// and pretend the request didn't fail
// If IOP_MONITOR is defined, then it doesn't matter whether IOP_MOCK_MONITOR is
// defined
#define IOP_MOCK_MONITOR

enum InterruptEvent { NONE, FACTORY_RESET, ON_CONNECTION, MUST_UPGRADE };

static volatile enum InterruptEvent interruptEvent = NONE;

#define MAYBE_PROGMEM_STRING_EMPTY(name) static const Option<StaticString> name;
#define MAYBE_PROGMEM_STRING(name, msg)                                        \
  PROGMEM_STRING(name_##storage, msg);                                         \
  static const Option<StaticString> name(name_##storage);

#include "Stream.h"
#include "WString.h"

bool runUpdate(Stream &in, uint32_t size, const String &md5,
               int command) noexcept;

#include <memory>

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&...args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif