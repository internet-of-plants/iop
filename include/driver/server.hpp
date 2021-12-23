#ifndef IOP_DRIVER_SERVER_HPP
#define IOP_DRIVER_SERVER_HPP

#include "driver/log.hpp"
#include <functional>
#include <unordered_map>
#include <memory>

#ifdef IOP_DESKTOP
class sockaddr_in
#else
class DNSServer;
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
  void *server; // ESP8266WebServer
public:
  HttpConnection(void *parent) noexcept;
  ~HttpConnection() noexcept;
  
  HttpConnection(HttpConnection &other) noexcept = delete;
  HttpConnection(HttpConnection &&other) noexcept;
  auto operator=(HttpConnection &other) noexcept -> HttpConnection & = delete;
  auto operator=(HttpConnection &&other) noexcept -> HttpConnection &;
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
  sockaddr_in *address;
#else
  void *server; // ESP8266WebServer
public:
  HttpServer(HttpServer &other) noexcept = delete;
  HttpServer(HttpServer &&other) noexcept;
  auto operator=(HttpServer &other) noexcept -> HttpServer & = delete;
  auto operator=(HttpServer &&other) noexcept -> HttpServer &;
#endif
public:
  HttpServer(uint32_t port = 8082) noexcept;
  ~HttpServer() noexcept;

  void begin() noexcept;
  void close() noexcept;
  void handleClient() noexcept;
  void on(iop::StaticString uri, Callback handler) noexcept;
  void onNotFound(Callback fn) noexcept;
};

class CaptivePortal {
  DNSServer *server;
  
public:
  CaptivePortal() noexcept: server(nullptr) {}
  ~CaptivePortal() noexcept;

  CaptivePortal(CaptivePortal &other) noexcept = delete;
  CaptivePortal(CaptivePortal &&other) noexcept;
  auto operator=(CaptivePortal &other) noexcept -> CaptivePortal & = delete;
  auto operator=(CaptivePortal &&other) noexcept -> CaptivePortal &;

  void start() noexcept;
  void close() noexcept;  
  void handleClient() const noexcept;
};
}
#endif