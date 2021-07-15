#ifndef IOP_UTILS_HPP
#define IOP_UTILS_HPP

#include "core/log.hpp"
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
#ifdef IOP_DESKTOP
#define IOP_NETWORK_LOGGING
#endif

// (Un)Comment this line to toggle sensors dependency
#ifndef IOP_DESKTOP
//#define IOP_SENSORS
#endif

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
//#define IOP_MOCK_MONITOR

#ifdef IOP_MONITOR
#undef IOP_MOCK_MONITOR
#endif

#ifndef IOP_SERIAL
#ifdef IOP_NETWORK_LOGGING
#undef IOP_NETWORK_LOGGING
#endif
#endif

// If you change the number of interrupt types, please update interruptVariant
// to the correct size
enum class InterruptEvent { NONE, FACTORY_RESET, ON_CONNECTION, MUST_UPGRADE };
constexpr static uint8_t interruptVariants = 4;

namespace panic {
  void setup() noexcept;
}
namespace network_logger {
  void setup() noexcept;
}

/// Abstracts factory resets
namespace reset {
void setup() noexcept;
} // namespace reset


namespace utils {
void scheduleInterrupt(InterruptEvent ev) noexcept;
auto descheduleInterrupt() noexcept -> InterruptEvent;
auto base64Encode(const uint8_t *in, const size_t size) noexcept -> std::string;
} // namespace utils


// TODO: make it type-safer
using AuthToken = std::array<char, 64>;
using NetworkName = std::array<char, 32>;
using NetworkPassword = std::array<char, 64>;

struct PanicData {
  std::string_view msg;
  iop::StaticString file;
  uint32_t line;
  iop::StaticString func;
};

class WifiCredentials {
public:
  std::reference_wrapper<NetworkName> ssid;
  std::reference_wrapper<NetworkPassword> password;

  ~WifiCredentials() noexcept = default;
  WifiCredentials(NetworkName &ssid, NetworkPassword &pass) noexcept
      : ssid(std::ref(ssid)), password(std::ref(pass)) {}
  WifiCredentials(const WifiCredentials &cred) noexcept = default;
  WifiCredentials(WifiCredentials &&cred) noexcept = default;
  auto operator=(const WifiCredentials &cred) noexcept
      -> WifiCredentials & = default;
  auto operator=(WifiCredentials &&cred) noexcept
      -> WifiCredentials & = default;
};

struct EventStorage {
  float airTemperatureCelsius;
  float airHumidityPercentage;
  float airHeatIndexCelsius;
  uint16_t soilResistivityRaw;
  float soilTemperatureCelsius;
};

class Event {
public:
  EventStorage storage;
  ~Event() noexcept { IOP_TRACE(); }
  explicit Event(EventStorage storage) noexcept : storage(storage) {
    IOP_TRACE();
  }
  Event(Event const &ev) noexcept : storage(ev.storage) { IOP_TRACE(); }
  Event(Event &&ev) noexcept : storage(ev.storage) { IOP_TRACE(); }
  auto operator=(Event const &ev) noexcept -> Event & {
    IOP_TRACE();
    if (this == &ev)
      return *this;
    this->storage = ev.storage;
    return *this;
  }
  auto operator=(Event &&ev) noexcept -> Event & {
    IOP_TRACE();
    this->storage = ev.storage;
    return *this;
  }
};

#endif