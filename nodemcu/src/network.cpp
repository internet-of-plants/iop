#include <certificate_storage.hpp>
#include <network.hpp>
#include "generated/certificates.h"
#include <memory>

StaticString Network::wifiCodeToString(const wl_status_t val) const {
  switch (val) {
    case WL_NO_SHIELD:
      return F("WL_NO_SHIELD");
    case WL_IDLE_STATUS:
      return F("WL_IDLE_STATUS");
    case WL_NO_SSID_AVAIL:
      return F("WL_NO_SSID_AVAIL");
    case WL_SCAN_COMPLETED:
      return F("WL_SCAN_COMPLETED");
    case WL_CONNECTED:
      return F("WL_CONNECTED");
    case WL_CONNECT_FAILED:
      return F("WL_CONNECT_FAILED");
    case WL_CONNECTION_LOST:
      return F("WL_CONNECTION_LOST");
    case WL_DISCONNECTED:
      return F("WL_DISCONNECTED");
    default:
      panic_("Unrecognized wifi status code: " + String(val));
  }
}

#ifndef IOP_NETWORK_DISABLED
#include <ESP8266HTTPClient.h>

String Network::macAddress() const { return WiFi.macAddress(); }
void Network::disconnect() const { WiFi.disconnect(); }

// TODO: connections aren't working with AP active

void Network::setup() const {
  #ifdef IOP_ONLINE
    // Makes sure the event handler is never dropped and only set once
    static const auto handler = WiFi.onStationModeGotIP([=](const WiFiEventStationModeGotIP &ev) {
      interruptEvent = ON_CONNECTION;
      (void) ev;
    });
    if (this->isConnected()) {
      interruptEvent = ON_CONNECTION;
    }
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    WiFi.mode(WIFI_STA);
    station_config status = {0};
    wifi_station_get_config_default(&status);
    if (status.ssid != NULL || !this->isConnected()) {
      WiFi.waitForConnectResult();
    }
  #else
    WiFi.mode(WIFI_OFF);
  #endif
}

bool Network::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

Option<Response> Network::httpPut(const StringView token, const StaticString path, const StringView data) const {
  return this->httpRequest(PUT, token, path, data);
}

Option<Response> Network::httpPost(const StringView token, const StaticString path, const StringView data) const {
  return this->httpRequest(POST, token, path, data);
}

Option<Response> Network::httpPost(const StaticString path, const StringView data) const {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}

StaticString methodToString(const enum HttpMethod method) {
  switch (method) {
    case GET:
      return F("GET");
    case DELETE:
      return F("DELETE");
    case POST:
      return F("POST");
    case PUT:
      return F("PUT");
  }
  panic_(F("Invalid Method"));
}

auto certStore = std::unique_ptr<BearSSL::CertStore>(new BearSSL::CertStore());
auto client = std::unique_ptr<WiFiClientSecure>(new WiFiClientSecure());
auto http = std::unique_ptr<HTTPClient>(new HTTPClient());
Option<Response> Network::httpRequest(const enum HttpMethod method, const Option<StringView> token, const StaticString path, Option<StringView> data) const {
  //const String uri = String(this->host_.get()) + String(FPSTR(path.get()));
  //const auto port 4001;
  const String uri = "https://google.com";
  const auto port = 443;
  const auto data_ = data.unwrapOr(StaticString(F("")));
  this->logger.info(methodToString(method), START, F(" "));
  this->logger.info(uri, CONTINUITY);
  this->logger.info(data_);

  if (this->isConnected()) {
    // We should make sure our server supports Max Fragment Length Negotiation
    // if (client->probeMaxFragmentLength(uri, port, 512)) {
    //   client->setBufferSizes(512, 512);
    // }
    client->setNoDelay(false);
    client->setSync(true);  
    client->setCertStore(certStore.get());
    //client->setInsecure();
    if (client->connect(uri, port) <= 0) {
      this->logger.warn(F("Failed to connect to"), START, F(" "));
      this->logger.warn(uri, CONTINUITY);
      delay(200);
      return Option<Response>();
    }
    client->disableKeepAlive();

    if (!http->begin(*client, uri)) {
      this->logger.warn(F("Failed to begin http connection to"), START, F(" "));
      this->logger.warn(uri, CONTINUITY);
      delay(200);
      return Option<Response>();
    }
    http->setReuse(false);

    http->addHeader(F("Connection"), F("close"));
    if (data.isSome()) {
      http->addHeader(F("Content-Type"), F("application/json"));
    }
    if (token.isSome()) {
      const StringView& tok = token.asRef().expect(F("Missing auth token when it shouldn't"));
      http->addHeader(F("Authorization"), String("Basic ") + String(tok.get()));
    }

    const int httpCode = http->sendRequest(methodToString(method).asCharPtr(), reinterpret_cast<const uint8_t *>(data_.get()), strlen(data_.get()));

    this->logger.info(F("Response code:"), START, F(" "));
    this->logger.info(String(httpCode), CONTINUITY);
    if (httpCode < 0 || httpCode >= UINT16_MAX) {
      this->logger.error(F("Connection failed"));
      delay(200);
      return Option<Response>();
    }

    if (http->getSize() > 2048) {
      this->logger.error(F("Payload from server was too big:"), START, F(" "));
      this->logger.error(String(http->getSize()), CONTINUITY);
      return Option<Response>();
    }

    const auto payload = http->getString();
    return Option<Response>((Response) {
      .code = (uint16_t) httpCode,
      .payload = payload,
    });
  }

  return Option<Response>();
}
#endif

#ifdef IOP_NETWORK_DISABLED
void Network::setup() const {}
bool Network::isConnected() const { return true; }
Option<Response> Network::httpPut(const StringView token, const StaticString path, const StringView data) const {
  return this->httpRequest(PUT, token, path, data);
}
Option<Response> Network::httpPost(const StringView token, const StaticString path, const StringView data) const {
  return this->httpRequest(POST, token, path, data);
}
Option<Response> Network::httpPost(const StaticString path, const StringView data) const {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}
Option<Response> Network::httpRequest(const enum HttpMethod method, const Option<StringView> token, const StaticString path, Option<StringView> data) const {
  (void) method;
  (void) token;
  (void) path;
  (void) data;
  return Option<Response>((Response) {
    .code = 200,
    .payload = "",
  });
}
String Network::macAddress() const { return "MAC::MAC::ERS::ON"; }
void Network::disconnect() const {}
#endif
