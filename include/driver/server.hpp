#ifndef IOP_DRIVER_SERVER
#define IOP_DRIVER_SERVER

#include "core/string.hpp"
#include "core/log.hpp"
#include <functional>
#include <unordered_map>
#include <memory>

#ifdef IOP_DESKTOP
#include <netinet/in.h>
#else
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#endif

namespace driver {
class HttpConnection {
// TODO: make variables private with setters/getters available to friend classes (make HttpServer a friend)
public:
#ifdef IOP_DESKTOP
  std::optional<int32_t> currentClient;
  std::string currentHeaders;
  std::string currentPayload;
  std::optional<size_t> currentContentLength;
  std::string currentRoute;
  
  using Buffer = std::array<char, 1024>;
#else
private:
  std::reference_wrapper<ESP8266WebServer> server;
public:
  HttpConnection(ESP8266WebServer &parent) noexcept: server(parent) {}
#endif

  auto arg(iop::StaticString arg) const noexcept -> std::optional<std::string>;
  void sendHeader(iop::StaticString name, iop::StaticString value) noexcept;
  void send(uint16_t code, iop::StaticString type, iop::StaticString data) const noexcept;
  void sendData(iop::StaticString data) const noexcept;
  void setContentLength(size_t length) noexcept;
  void reset() noexcept;
};

class HttpServer {
  // TODO: currently we only support one client at a time, this is not thread safe tho
  bool isHandlingRequest = false;
public:
  using Callback = std::function<void(HttpConnection&, iop::Log const &)>;
#ifdef IOP_DESKTOP
private:
  std::unordered_map<std::string, Callback> router;
  Callback notFoundHandler;
  uint32_t port;

  std::optional<uint32_t> maybeFD;
  std::optional<sockaddr_in> maybeAddress;
#else
  std::unique_ptr<ESP8266WebServer> _server;
  ESP8266WebServer & server() noexcept;
#endif
public:
  HttpServer(uint32_t port = 8082) noexcept;
  void begin() noexcept;
  void close() noexcept;
  void handleClient() noexcept;
  void on(iop::StaticString uri, Callback handler) noexcept;
  void onNotFound(Callback fn) noexcept;
};

class CaptivePortal {
  std::unique_ptr<DNSServer> server;
  
public:
  void start() noexcept;
  void close() noexcept;  
  void handleClient() const noexcept;
};
}
#endif