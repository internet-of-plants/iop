#ifndef IOP_API_H_
#define IOP_API_H_

#include <models.hpp>
#include <option.hpp>
#include <result.hpp>
#include <network.hpp>
#include <log.hpp>

class Api {
private:
  String host_;
  Log logger;
  Network network;

public:
  Api(const String host, const LogLevel logLevel):
    host_(host),
    logger(logLevel, "API"),
    network(host, logLevel) {}
  Api(Api& other) = delete;
  void operator=(Api& other) = delete;
  void operator=(Api&& other) {
    this->host_ = std::move(other.host_);
    this->logger = std::move(other.logger);
    this->network = std::move(other.network);
  }
  Api(Api&& other):
    host_(std::move(other.host_)),
    logger(other.logger.level(), other.logger.target()),
    network(std::move(other.network)) {}


  void setup(std::function<void (const WiFiEventStationModeGotIP &)> onConnection) const;
  Option<uint16_t> registerEvent(const AuthToken token, const Event event) const;
  Result<PlantId, Option<uint16_t>> registerPlant(const String token) const;
  Option<AuthToken> authenticate(const String username, const String password) const;
  String host() const { return this->host_; }
  bool isConnected() const { return this->network.isConnected(); }
  String macAddress() const { return this->network.macAddress(); }
  void disconnect() const { this->network.disconnect(); }
  LogLevel loggerLevel() const { return this->logger.level(); }
};

#endif