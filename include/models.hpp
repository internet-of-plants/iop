#ifndef IOP_MODELS_HPP
#define IOP_MODELS_HPP

#include "certificate_storage.hpp"

#include "result.hpp"
#include "static_string.hpp"
#include "storage.hpp"
#include "string_view.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

#include <array>
#include <cstdint>
#include <memory>

// Those are basically utils::Storage but typesafe
TYPED_STORAGE(AuthToken, 64);
TYPED_STORAGE(NetworkName, 32);
TYPED_STORAGE(NetworkPassword, 64);
TYPED_STORAGE(MD5Hash, 32);
TYPED_STORAGE(MacAddress, 17);

class Log;

namespace utils {
auto hashSketch() noexcept -> MD5Hash;
auto macAddress() noexcept -> MacAddress;
void ICACHE_RAM_ATTR scheduleInterrupt(InterruptEvent ev) noexcept;
auto ICACHE_RAM_ATTR descheduleInterrupt() noexcept -> InterruptEvent;
void logMemory(const Log &logger) noexcept;
} // namespace utils

struct PanicData {
  // TODO: this could have a StaticString alternative to be able to use `_P`
  // PROGMEM methods
  StringView msg;
  StaticString file;
  uint32_t line;
  StringView func;
};

struct WifiCredentials {
  NetworkName ssid;
  NetworkPassword password;
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
  MacAddress mac;
  MD5Hash firmwareHash;
  ~Event() { IOP_TRACE(); }
  Event(EventStorage storage, MacAddress mac, MD5Hash firmwareHash) noexcept
      : storage(storage), mac(std::move(mac)),
        firmwareHash(std::move(firmwareHash)) {
    IOP_TRACE();
  }
  Event(Event const &ev)
      : storage(ev.storage), mac(ev.mac), firmwareHash(ev.firmwareHash) {
    IOP_TRACE();
  }
  Event(Event &&ev)
      : storage(std::move(ev.storage)), mac(std::move(ev.mac)),
        firmwareHash(std::move(ev.firmwareHash)) {
    IOP_TRACE();
  }
  auto operator=(Event const &ev) -> Event & {
    IOP_TRACE();
    this->storage = ev.storage;
    this->mac = ev.mac;
    this->firmwareHash = ev.firmwareHash;
    return *this;
  }
  auto operator=(Event &&ev) noexcept -> Event & {
    IOP_TRACE();
    this->storage = std::move(ev.storage);
    this->mac = std::move(ev.mac);
    this->firmwareHash = std::move(ev.firmwareHash);
    return *this;
  }
};

#endif
