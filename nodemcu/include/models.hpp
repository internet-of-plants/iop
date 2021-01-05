#ifndef IOP_MODELS_H
#define IOP_MODELS_H

#include <array>
#include <cstdint>
#include <memory>

#include "result.hpp"
#include "storage.hpp"
#include "string_view.hpp"

// Those are basically utils::Storage but typesafe
TYPED_STORAGE(AuthToken, 64);
TYPED_STORAGE(PlantId, 19);
TYPED_STORAGE(NetworkName, 32);
TYPED_STORAGE(NetworkPassword, 64);
TYPED_STORAGE(MD5Hash, 32);

MD5Hash hashSketch() noexcept;

typedef uint16_t HttpCode;

typedef struct panic_data_ {
  // TODO: this could have a StaticString alternative to be able to use `_P`
  // PROGMEM methods
  StringView msg;
  StaticString file;
  uint32_t line;
  StringView func;
} PanicData;

typedef struct wifiCredentials {
  NetworkName ssid;
  NetworkPassword password;
} WifiCredentials;

typedef struct eventStorage {
  float airTemperatureCelsius;
  float airHumidityPercentage;
  float airHeatIndexCelsius;
  uint16_t soilResistivityRaw;
  float soilTemperatureCelsius;
} EventStorage;

class Event {
public:
  const EventStorage storage;
  const PlantId plantId;
  const MD5Hash firmwareHash;
  Event(const EventStorage storage, const PlantId plantId,
        const MD5Hash firmwareHash) noexcept
      : storage(storage), plantId(std::move(plantId)),
        firmwareHash(std::move(firmwareHash)) {}
  Event(Event &&ev)
      : storage(ev.storage), plantId(std::move(plantId)),
        firmwareHash(std::move(firmwareHash)) {}
};

#endif
