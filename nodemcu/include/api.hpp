#ifndef IOP_API_H
#define IOP_API_H

#include "fixed_string.hpp"
#include "log.hpp"
#include "models.hpp"
#include "network.hpp"
#include "option.hpp"
#include "result.hpp"
#include "static_string.hpp"
#include "storage.hpp"
#include "string_view.hpp"

#include "ArduinoJson.h"

/// Abstracts Internet of Plants API to avoid mistakes and properly report
/// errors
class Api {
private:
  Log logger;
  Network network_;

public:
  Api(const StaticString host, const LogLevel logLevel) noexcept
      : logger(logLevel, F("API")), network_(host, logLevel) {}
  Api(Api &other) = delete;
  Api(Api &&other) = delete;
  Api &operator=(Api &other) = delete;
  Api &operator=(Api &&other) = delete;

  void setup() const noexcept;
  ApiStatus upgrade(const AuthToken &token,
                    const MD5Hash sketchHash) const noexcept;
  ApiStatus reportPanic(const AuthToken &authToken, const Option<PlantId> &id,
                        const PanicData &event) const noexcept;
  ApiStatus registerEvent(const AuthToken &token,
                          const Event &event) const noexcept;
  Result<PlantId, ApiStatus>
  registerPlant(const AuthToken &token) const noexcept;
  Result<AuthToken, ApiStatus>
  authenticate(const StringView username,
               const StringView password) const noexcept;
  ApiStatus reportError(const AuthToken &authToken, const PlantId &id,
                        const StringView error) const noexcept;
  StaticString host() const noexcept { return this->network().host(); }
  bool isConnected() const noexcept;
  String macAddress() const noexcept;
  void disconnect() const noexcept;
  LogLevel loggerLevel() const noexcept;

  const Network &network() const noexcept { return this->network_; }

private:
  using JsonCallback = std::function<void(JsonDocument &)>;
  template <uint16_t SIZE>
  Option<FixedString<SIZE>> makeJson(const StaticString name,
                                     const JsonCallback func) const noexcept {
    auto doc = make_unique<StaticJsonDocument<SIZE>>();
    func(*doc);

    if (doc->overflowed()) {
      const auto s = std::to_string(SIZE);
      this->logger.error(F("Payload doesn't fit Json<"), s, F("> at "), name);
      return Option<FixedString<SIZE>>();
    }

    auto buffer = FixedString<SIZE>::empty();
    serializeJson(*doc, buffer.asMut(), buffer.size);
    this->logger.debug(F("Json: "), *buffer);
    return Option<FixedString<SIZE>>(buffer);
  }
};

#include "utils.hpp"
#ifdef IOP_MONITOR
#undef IOP_MOCK_MONITOR
#endif
#ifndef IOP_MOCK_MONITOR
#ifndef IOP_MONITOR
#define IOP_API_DISABLED
#endif
#endif

#endif