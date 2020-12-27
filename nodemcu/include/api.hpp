#ifndef IOP_API_H_
#define IOP_API_H_

#include <models.hpp>
#include <option.hpp>
#include <storage.hpp>
#include <result.hpp>
#include <network.hpp>
#include <log.hpp>
#include <string_view.hpp>
#include <static_string.hpp>

/// Abstracts Internet of Plants API to avoid mistakes and properly report errors
class Api {
private:
  Log logger;
  Network network;

public:
  Api(const StaticString host, const LogLevel logLevel) noexcept:
    logger(logLevel, F("API")),
    network(host, logLevel) {}
  Api(Api& other) = delete;
  Api(Api&& other) = delete;
  Api& operator=(Api& other) = delete;
  Api& operator=(Api&& other) = delete;

  void setup() const noexcept;
  Option<HttpCode> reportPanic(const AuthToken & authToken, Option<PlantId> id, const PanicData & event) const noexcept;
  Option<HttpCode> registerEvent(const AuthToken & token, const Event & event) const noexcept;
  Result<PlantId, Option<HttpCode>> registerPlant(const AuthToken & token) const noexcept;
  Result<AuthToken, Option<HttpCode>> authenticate(const StringView username, const StringView password) const noexcept;
  Option<HttpCode> reportError(const AuthToken &authToken, const PlantId &id, const StringView error) const noexcept;
  StaticString host() const noexcept { return this->network.host(); }
  bool isConnected() const noexcept;
  String macAddress() const noexcept;
  void disconnect() const noexcept;
  LogLevel loggerLevel() const noexcept;
};

#include <utils.hpp>
#ifdef IOP_MONITOR
  #undef IOP_MOCK_MONITOR
#endif
#ifndef IOP_MOCK_MONITOR
#ifndef IOP_MONITOR
  #define IOP_API_DISABLED
#endif
#endif

#endif