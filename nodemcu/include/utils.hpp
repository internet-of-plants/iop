#ifndef IOP_UTILS_H_
#define IOP_UTILS_H_

#include <memory>
#include <cstdint>
#include <array>
#include <WString.h>

#include <result.hpp>
#include <string_view.hpp>
#include <option.hpp>
#include <fixed_string.hpp>
#include <models.hpp>

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

// (Un)Comment this line to toggle memory stats logging
//#define LOG_MEMORY


// If IOP_MONITOR is not defined the Api methods will be short-circuited
// If IOP_MOCK_MONITOR is defined, then the methods will run normally
// and pretend the request didn't fail
// If IOP_MONITOR is defined, then it doesn't matter whether IOP_MOCK_MONITOR is defined
#define IOP_MOCK_MONITOR

enum InterruptEvent {
  NONE,
  FACTORY_RESET,
  ON_CONNECTION
};

static volatile enum InterruptEvent interruptEvent = NONE;

namespace utils {
  uint64_t hashString(const StringView string);
  Option<size_t> search(const StringView haystack, const StaticString& needle);
  Option<size_t> search(const StringView haystack, const StaticString& needle, const size_t fromIndex);
  Option<size_t> search(const StringView haystack, const StringView needle);
  Option<size_t> search(const StringView haystack, const StringView needle, const size_t fromIndex);
  uint64_t u64fromString(const StringView str);
  uint64_t random();
}


#define MAYBE_PROGMEM_STRING_EMPTY(name) static const Option<StaticString> name;
#define MAYBE_PROGMEM_STRING(name, msg) PROGMEM_STRING(name_##storage, msg);\
  static const Option<StaticString> name(name_##storage);

#endif