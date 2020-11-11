#ifndef IOP_MODELS_H_
#define IOP_MODELS_H_

#include <array>
#include <cstdint>
#include <memory>

#include <result.hpp>
#include <string_view.hpp>

enum ParseError {
  TOO_BIG
};

class AuthToken {
public:
  static constexpr const size_t dataSize = 64;
  static constexpr const size_t size = dataSize + 1;
  typedef std::array<uint8_t, AuthToken::size> Storage;
private:
  std::shared_ptr<Storage> token;

public:
  AuthToken(const Storage token): token(std::make_shared<Storage>(token)) {
    *this->token->end() = 0;
  }
  AuthToken(std::shared_ptr<Storage> token): token(std::move(token)) {
    *this->token->end() = 0;
  }
  AuthToken(const AuthToken& other): token(other.token) {}
  AuthToken(AuthToken&& other): token(std::move(other.token)) {}
  void operator=(const AuthToken& other) { this->token = other.token; }
  void operator=(AuthToken&& other) { this->token = std::move(other.token); }
  const uint8_t * constPtr() const { return this->token->data(); }
  uint8_t * mutPtr() { return this->token->data(); }
  StringView asString() const { return (const char *) this->token->data(); }

  static Result<AuthToken, enum ParseError> fromString(const StringView str) {
    if (strlen(str.get()) > AuthToken::dataSize) {
      return Result<AuthToken, ParseError>(ParseError::TOO_BIG);
    }
    auto token = std::make_shared<AuthToken::Storage>();
    token->fill(0);
    memcpy(token.get(), str.get(), AuthToken::dataSize);
    return Result<AuthToken, enum ParseError>(std::move(token));
  }
};

class PlantId {
public:
  static constexpr const size_t dataSize = 19;
  static constexpr const size_t size = dataSize + 1;
  typedef std::array<uint8_t, PlantId::size> Storage;
private:
  std::shared_ptr<Storage> id;

public:
  PlantId(const Storage id): id(std::make_shared<Storage>(id)) {
    *this->id->end() = 0;
  }
  PlantId(std::shared_ptr<Storage> id): id(std::move(id)) {
    *this->id->end() = 0;
  }
  PlantId(const PlantId& other): id(other.id) {}
  PlantId(PlantId&& other): id(std::move(other.id)) {}
  void operator=(const PlantId& other) { this->id = other.id; }
  void operator=(PlantId&& other) { this->id = std::move(other.id); }
  const uint8_t * constPtr() const { return this->id->data(); }
  uint8_t * mutPtr() { return this->id->data(); }
  StringView asString() const { return (const char *) this->id->data(); }

  static Result<PlantId, enum ParseError> fromString(const StringView str) {
    if (strlen(str.get()) > PlantId::dataSize) {
      return Result<PlantId, ParseError>(ParseError::TOO_BIG);
    }
    auto id = std::make_shared<PlantId::Storage>();
    id->fill(0);
    memcpy(id.get(), str.get(), PlantId::dataSize);
    return Result<PlantId, enum ParseError>(std::move(id));
  }
};

class NetworkName {
public:
  static constexpr const size_t dataSize = 32;
  static constexpr const size_t size = dataSize + 1;
  typedef std::array<uint8_t, NetworkName::size> Storage;
private:
  std::shared_ptr<Storage> ssid;

public:
  NetworkName(const Storage ssid): ssid(std::make_shared<Storage>(ssid)) {
    *this->ssid->end() = 0;
  }
  NetworkName(std::shared_ptr<Storage> ssid): ssid(std::move(ssid)) {
    *this->ssid->end() = 0;
  }
  NetworkName(const NetworkName& other): ssid(other.ssid) {}
  NetworkName(NetworkName&& other): ssid(std::move(other.ssid)) {}
  void operator=(const NetworkName& other) { this->ssid = other.ssid; }
  void operator=(NetworkName&& other) { this->ssid = std::move(other.ssid); }
  const uint8_t * constPtr() const { return this->ssid->data(); }
  uint8_t * mutPtr() { return this->ssid->data(); }
  StringView asString() const { return (const char *) this->ssid->data(); }

  static Result<NetworkName, enum ParseError> fromString(const StringView str) {
    if (strlen(str.get()) > NetworkName::dataSize) {
      return Result<NetworkName, ParseError>(ParseError::TOO_BIG);
    }
    auto token = std::make_shared<NetworkName::Storage>();
    token->fill(0);
    memcpy(token.get(), str.get(), NetworkName::dataSize);
    return Result<NetworkName, enum ParseError>(std::move(token));
  }
};

class NetworkPassword {
public:
  static constexpr const size_t dataSize = 64;
  static constexpr const size_t size = dataSize + 1;
  typedef std::array<uint8_t, NetworkPassword::size> Storage;
private:
  std::shared_ptr<Storage> psk;

public:
  NetworkPassword(const Storage psk): psk(std::make_shared<Storage>(psk)) {
    *this->psk->end() = 0;
  }
  NetworkPassword(std::shared_ptr<Storage> psk): psk(std::move(psk)) {
    *this->psk->end() = 0;
  }
  NetworkPassword(const NetworkPassword& other): psk(other.psk) {}
  NetworkPassword(NetworkPassword&& other): psk(std::move(other.psk)) {}
  void operator=(const NetworkPassword& other) { this->psk = other.psk; }
  void operator=(NetworkPassword&& other) { this->psk = std::move(other.psk); }
  const uint8_t * constPtr() const { return this->psk->data(); }
  uint8_t * mutPtr() { return this->psk->data(); }
  StringView asString() const { return (const char *) this->psk->data(); }

  static Result<NetworkPassword, enum ParseError> fromString(const StringView str) {
    if (strlen(str.get()) > NetworkPassword::dataSize) {
      return Result<NetworkPassword, ParseError>(ParseError::TOO_BIG);
    }
    auto token = std::make_shared<NetworkPassword::Storage>();
    token->fill(0);
    memcpy(token.get(), str.get(), NetworkPassword::dataSize);
    return Result<NetworkPassword, enum ParseError>(std::move(token));
  }
};

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
