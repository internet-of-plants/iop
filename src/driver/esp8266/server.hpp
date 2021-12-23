#include "driver/server.hpp"
#include "driver/panic.hpp"
#include "ESP8266WebServer.h"
#include "DNSServer.h"
#include <user_interface.h>
#include <optional>

namespace driver {
iop::Log & logger() noexcept {
  static iop::Log logger_(iop::LogLevel::WARN, FLASH("HTTP Server"));
  return logger_;
}

auto to_server(void *ptr) noexcept -> ESP8266WebServer & {
  return *reinterpret_cast<ESP8266WebServer *>(ptr);
}

HttpConnection::HttpConnection(void * parent) noexcept: server(parent) {}
auto HttpConnection::arg(iop::StaticString arg) const noexcept -> std::optional<std::string> {
  if (!to_server(this->server).hasArg(arg.get())) return std::optional<std::string>();
  return std::string(to_server(this->server).arg(arg.get()).c_str());
}
void HttpConnection::sendHeader(iop::StaticString name, iop::StaticString value) noexcept {
  to_server(this->server).sendHeader(String(name.get()), String(value.get()));
}
void HttpConnection::send(uint16_t code, iop::StaticString type, iop::StaticString data) const noexcept {
  to_server(this->server).send_P(code, type.asCharPtr(), data.asCharPtr());
}
void HttpConnection::sendData(iop::StaticString data) const noexcept {
  to_server(this->server).sendContent_P(data.asCharPtr());
}
void HttpConnection::setContentLength(size_t length) noexcept {
  to_server(this->server).setContentLength(length);
}
void HttpConnection::reset() noexcept {}

static uint32_t serverPort = 0;
HttpServer::HttpServer(uint32_t port) noexcept { IOP_TRACE(); serverPort = port; }

auto validateServer(void **ptr) noexcept -> ESP8266WebServer & {
  if (!*ptr) {
    iop_assert(serverPort != 0, FLASH("Server port is not defined"));
    ESP8266WebServer **s = reinterpret_cast<ESP8266WebServer **>(ptr);
    *s = new (std::nothrow) ESP8266WebServer(serverPort);
  }
  iop_assert(*ptr, FLASH("Unable to allocate ESP8266WebServer"));
  return to_server(*ptr);
}
HttpConnection::HttpConnection(HttpConnection &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto HttpConnection::operator=(HttpConnection &&other) noexcept -> HttpConnection & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}
HttpConnection::~HttpConnection() noexcept {
  delete reinterpret_cast<ESP8266WebServer *>(this->server);
}
HttpServer::HttpServer(HttpServer &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto HttpServer::operator=(HttpServer &&other) noexcept -> HttpServer & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}
HttpServer::~HttpServer() noexcept {
  delete reinterpret_cast<ESP8266WebServer *>(this->server);
}
void HttpServer::begin() noexcept { IOP_TRACE(); validateServer(&this->server).begin(); }
void HttpServer::close() noexcept { IOP_TRACE(); validateServer(&this->server).close(); }
void HttpServer::handleClient() noexcept {
  IOP_TRACE();
  iop_assert(!this->isHandlingRequest, FLASH("Already handling a client"));
  this->isHandlingRequest = true;
  validateServer(&this->server).handleClient();
  this->isHandlingRequest = false;
}
void HttpServer::on(iop::StaticString uri, Callback handler) noexcept {
  IOP_TRACE();
  auto *_server = &driver::validateServer(&this->server);
  validateServer(&this->server).on(uri.get(), [handler, _server]() { HttpConnection conn(_server); handler(conn, logger()); });
}
void HttpServer::onNotFound(Callback handler) noexcept {
  IOP_TRACE();
  auto *_server = &driver::validateServer(&this->server);
  validateServer(&this->server).onNotFound([handler, _server]() { HttpConnection conn(_server); handler(conn, logger()); });
}
CaptivePortal::CaptivePortal(CaptivePortal &&other) noexcept: server(other.server) {
  other.server = nullptr;
}
auto CaptivePortal::operator=(CaptivePortal &&other) noexcept -> CaptivePortal & {
  this->server = other.server;
  other.server = nullptr;
  return *this;
}

CaptivePortal::~CaptivePortal() noexcept {
  delete this->server;
}
void CaptivePortal::start() noexcept {
  const uint16_t port = 53;
  this->server = new (std::nothrow) DNSServer();
  iop_assert(this->server, FLASH("Unable to allocate DNSServer"));
  this->server->setErrorReplyCode(DNSReplyCode::NoError);
  this->server->start(port, FLASH("*").get(), ::WiFi.softAPIP());
}
void CaptivePortal::close() noexcept {
  iop_assert(this->server, FLASH("Must initialize DNSServer first"));
  this->server->stop();
  this->server = nullptr;
}
void CaptivePortal::handleClient() const noexcept {
  iop_assert(this->server, FLASH("Must initialize DNSServer first"));
  this->server->processNextRequest();
}
}