#ifndef IOP_API_H_
#define IOP_API_H_

#include <models.hpp>
#include <option.hpp>
#include <result.hpp>
#include <network.hpp>
#include <log.hpp>
#include <static_string.hpp>

class Api {
private:
  Log logger;
  Network network;

public:
  Api(const StaticString host, const LogLevel logLevel):
    logger(logLevel, STATIC_STRING("API")),
    network(host, logLevel) {}
  Api(Api& other) = delete;
  void operator=(Api& other) = delete;
  void operator=(Api&& other) {
    this->logger = std::move(other.logger);
    this->network = std::move(other.network);
  }
  Api(Api&& other):
    logger(other.logger.level(), other.logger.target()),
    network(std::move(other.network)) {}


  void setup() const;
  Option<HttpCode> registerEvent(const AuthToken & token, const Event & event) const;
  Result<PlantId, Option<HttpCode>> registerPlant(const AuthToken & token) const;
  Result<AuthToken, Option<HttpCode>> authenticate(const String & username, const String & password) const;
  StaticString host() const { return this->network.host(); }
  bool isConnected() const;
  String macAddress() const;
  void disconnect() const;
  LogLevel loggerLevel() const;
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