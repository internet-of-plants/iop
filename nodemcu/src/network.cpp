#include "network.hpp"
#include "certificate_storage.hpp"
#include "generated/certificates.h"

#include "static_string.hpp"
#include "string_view.hpp"

#include "panic.hpp"
#include <memory>

#ifndef IOP_NETWORK_DISABLED
#include "ESP8266HTTPClient.h"

String Network::macAddress() const noexcept { return WiFi.macAddress(); }
void Network::disconnect() const noexcept { WiFi.disconnect(); }

// TODO: connections aren't working with AP active, maybe because of the DNS
// hijack of server.cpp?

void Network::setup() const noexcept {
#ifdef IOP_ONLINE
  // Makes sure the event handler is never dropped and only set once
  static const auto callback = [](const WiFiEventStationModeGotIP &ev) {
    interruptEvent = ON_CONNECTION;
    (void)ev;
  };
  static const auto handler = WiFi.onStationModeGotIP(callback);

  if (this->isConnected()) {
    interruptEvent = ON_CONNECTION;
  }

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  // station_config status = {0};
  // wifi_station_get_config_default(&status);
  // if (status.ssid != NULL || !this->isConnected()) {
  //   WiFi.waitForConnectResult();
  // }
#else
  WiFi.mode(WIFI_OFF);
#endif
}

bool Network::isConnected() const noexcept {
  return WiFi.status() == WL_CONNECTED;
}

Option<Response> Network::httpPut(const StringView token,
                                  const StaticString path,
                                  const StringView data) const noexcept {
  return this->httpRequest(PUT, token, path, data);
}

Option<Response> Network::httpPost(const StringView token,
                                   const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, token, path, data);
}

Option<Response> Network::httpPost(const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}

Option<StaticString> methodToString(const enum HttpMethod method) noexcept {
  switch (method) {
  case GET:
    return StaticString(F("GET"));
  case HEAD:
    return StaticString(F("HEAD"));
  case POST:
    return StaticString(F("POST"));
  case PUT:
    return StaticString(F("PUT"));
  case PATCH:
    return StaticString(F("PATCH"));
  case DELETE:
    return StaticString(F("DELETE"));
  case CONNECT:
    return StaticString(F("CONNECT"));
  case OPTIONS:
    return StaticString(F("OPTIONS"));
  }
  return Option<StaticString>();
}

auto certStore = make_unique<BearSSL::CertStore>();
auto client = make_unique<WiFiClientSecure>();
auto http = std::make_shared<HTTPClient>();

Option<std::shared_ptr<HTTPClient>>
Network::httpClient(const StaticString path,
                    const Option<StringView> &token) const noexcept {
  if (!this->host_.contains(F(":"))) {
    PROGMEM_STRING(error, "Host must contain protocol (http:// or https://): ");
    panic_(String(error.get()) + this->host_.asCharPtr());
  }
  const auto uri = String(this->host_.get()) + path.get();
  this->logger.infoln(uri);
  constexpr const auto port = 4001;

  if (this->isConnected()) {
    // We should make sure our server supports Max Fragment Length Negotiation
    // if (client->probeMaxFragmentLength(uri, port, 512)) {
    //   client->setBufferSizes(512, 512);
    // }
    client->setNoDelay(false);
    client->setSync(true);
    client->setCertStore(certStore.get());
    // client->setInsecure();
    if (client->connect(uri, port) <= 0) {
      this->logger.warnln(F("Failed to connect to "), uri);
      return Option<std::shared_ptr<HTTPClient>>();
    }
    client->disableKeepAlive();

    if (!http->begin(*client, uri)) {
      this->logger.warnln(F("Failed to begin http connection to "), uri);
      return Option<std::shared_ptr<HTTPClient>>();
    }
    http->setReuse(false);

    if (token.isSome()) {
      const auto &tok = UNWRAP_REF(token);
      http->addHeader(F("Authorization"), String("Basic ") + String(tok.get()));
    }
  } else {
    return Option<std::shared_ptr<HTTPClient>>();
  }
  return http;
}

Option<Response>
Network::httpRequest(const enum HttpMethod method_,
                     const Option<StringView> &token, const StaticString path,
                     const Option<StringView> &data) const noexcept {
  if (this->httpClient(path, token).isNone()) {
    this->logger.errorln(F("Unable to obtain http client"));
    return Option<Response>();
  }

  StringView data_(StaticString(F("")));
  if (data.isSome()) {
    data_ = UNWRAP_REF(data);
  }

  const auto method = UNWRAP(methodToString(method_));

  this->logger.infoln(F("["), method, F("] "), path);
  // TODO: will this be a problem if we start streaming logs?
  // wifi credentials logged are a big no-no
  this->logger.debugln(F("Json data: "), data_);

  http->addHeader(F("Connection"), F("close"));
  if (data.isSome()) {
    http->addHeader(F("Content-Type"), F("application/json"));
  }

  const auto data__ = reinterpret_cast<const uint8_t *>(data_.get());
  const auto mtd = method.asCharPtr();

  const auto code = http->sendRequest(mtd, data__, data_.length());
  const auto strCode = std::to_string(code);

  this->logger.infoln(F("Response code: "), strCode);

  if (code < 0 || code >= UINT16_MAX) {
    this->logger.errorln(F("Connection failed. Code: "), strCode);
    return Option<Response>();
  }

  if (http->getSize() > 2048) {
    const auto lengthStr = std::to_string(http->getSize());
    this->logger.errorln(F("Payload from server was too big: "), lengthStr);
    return Option<Response>();
  }

  const auto payload = http->getString();
  return Option<Response>((Response){
      .code = (uint16_t)code,
      .payload = payload,
  });
}
#endif

#ifdef IOP_NETWORK_DISABLED
void Network::setup() const noexcept {}
bool Network::isConnected() const noexcept { return true; }
Option<Response> Network::httpPut(const StringView token,
                                  const StaticString path,
                                  const StringView data) const noexcept {
  return this->httpRequest(PUT, token, path, data);
}
Option<Response> Network::httpPost(const StringView token,
                                   const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, token, path, data);
}
Option<Response> Network::httpPost(const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}
Option<std::shared_ptr<HTTPClient>>
httpClient(const StaticString path,
           const Option<StringView> &token) const noexcept {
  (void)path;
  (void)token;
  return http;
}
Option<Response> Network::httpRequest(const enum HttpMethod method,
                                      const Option<StringView> token,
                                      const StaticString path,
                                      Option<StringView> data) const noexcept {
  (void)method;
  (void)token;
  (void)path;
  (void)data;
  return Option<Response>((Response){
      .code = 200,
      .payload = "",
  });
}
String Network::macAddress() const noexcept { return "MAC::MAC::ERS::ON"; }
void Network::disconnect() const noexcept {}
#endif