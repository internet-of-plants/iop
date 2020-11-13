#ifndef IOP_MODELS_H_
#define IOP_MODELS_H_

#include <array>
#include <cstdint>
#include <memory>

#include <result.hpp>
#include <string_view.hpp>
#include <storage.hpp>

// Those are basically utils::Storage but typesafe
TYPED_STORAGE(CsrfToken, 20);
TYPED_STORAGE(AuthToken, 64);
TYPED_STORAGE(PlantId, 19);
TYPED_STORAGE(NetworkName, 32);
TYPED_STORAGE(NetworkPassword, 64);

typedef uint16_t HttpCode;

struct WifiCredentials {
  NetworkName ssid;
  NetworkPassword password;
};

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
  Event(EventStorage storage, PlantId plantId):
    storage(storage),
    plantId(std::move(plantId)) {}
  Event(Event&& ev):
    storage(ev.storage),
    plantId(std::move(plantId)) {}
};

#endif
