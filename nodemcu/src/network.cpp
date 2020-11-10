#include <network.hpp>

String Network::wifiCodeToString(const wl_status_t val) const {
  switch (val) {
    case WL_NO_SHIELD:
      return "WL_NO_SHIELD";
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
    default:
      panic_("Unrecognized wifi status code: " + String(val));
  }
}

#ifndef IOP_NETWORK_DISABLED
#include <ESP8266HTTPClient.h>

String Network::macAddress() const { return WiFi.macAddress(); }
void Network::disconnect() const { WiFi.disconnect(); }


void Network::setup() const {
  #ifdef IOP_ONLINE
    // Makes sure the event handler is never dropped and only set once
    // static const auto handler = WiFi.onStationModeGotIP([=](const WiFiEventStationModeGotIP &ev) {
    //   interruptEvent = ON_CONNECTION;
    //   (void) ev;
    // });
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    WiFi.mode(WIFI_STA);
    station_config status = {0};
    wifi_station_get_config_default(&status);
    if (status.ssid != NULL && !this->isConnected()) {
      WiFi.waitForConnectResult();
    }
  #else
    WiFi.mode(WIFI_OFF);
  #endif
}

bool Network::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

Option<Response> Network::httpPut(const String token, const String path, const String data) const {
  return this->httpRequest(PUT, Option<String>(token), path, Option<String>(data));
}

Option<Response> Network::httpPost(const String token, const String path, const String data) const {
  return this->httpRequest(POST, Option<String>(token), path, Option<String>(data));
}

Option<Response> Network::httpPost(const String path, const String data) const {
  return this->httpRequest(POST, Option<String>(), path, Option<String>(data));
}

String methodToString(const enum HttpMethod method) {
  switch (method) {
    case GET:
      return "GET";
    case DELETE:
      return "DELETE";
    case POST:
      return "POST";
    case PUT:
      return "PUT";
  }
  panic_("Invalid Method");
}
auto client = std::unique_ptr<WiFiClientSecure>(new WiFiClientSecure());
auto http = std::unique_ptr<HTTPClient>(new HTTPClient());
Option<Response> Network::httpRequest(const enum HttpMethod method, Option<String> token, const String path, Option<String> data) const {
  const auto data_ = data.unwrap_or("");
  this->logger.info("[" + methodToString(method) + "] " + path + ": " + data_);

  const String uri = this->host_ + path;
  this->logger.warn(uri);
  if (this->isConnected()) {
    // We should make sure our server supports Max Fragment Length Negotiation
    // if (client->probeMaxFragmentLength(uri, 443, 512)) {
    //   client->setBufferSizes(512, 512);
    // }
    client->setNoDelay(false);
    client->setSync(true);    
    client->setInsecure(); // TODO: please fix it this is horrible
    if (client->connect("https://google.com", 443) <= 0) {
      this->logger.warn("Failed to connect to " + uri);
      delay(200);
      return Option<Response>();
    }
    client->disableKeepAlive();

    if (!http->begin(*client, uri)) {
      this->logger.warn("Failed to begin http connection to " + uri);
      delay(200);
      return Option<Response>();
    }
    http->setReuse(false);

    http->addHeader("Connection", "close");
    if (data.isSome()) {
      http->addHeader("Content-Type", "application/json");
    }
    if (token.isSome()) {
      http->addHeader("Authorization", "Basic " + token.expect("Missing auth token when it shouldn't"));
    }

    const int httpCode = http->sendRequest(methodToString(method).c_str(), (uint8_t *) data_.c_str(), data_.length());

    this->logger.info("Response code: " + String(httpCode));
    if (httpCode < 0 || httpCode >= UINT16_MAX) {
      this->logger.error("Connection failed");
      delay(200);
      return Option<Response>();
    }

    if (http->getSize() > 2048) {
      this->logger.error("Payload from server was too big: " + String(http->getSize()));
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
  Option<Response> Network::httpPut(const String token, const String path, const String data) const {
    return this->httpRequest(PUT, token, path, Option<String>(data));
  }
  Option<Response> Network::httpPost(const String token, const String path, const String data) const {
    return this->httpRequest(POST, token, path, Option<String>(data));
  }
  Option<Response> Network::httpPost(const String path, const String data) const {
    return this->httpRequest(POST, Option<String>(), path, data);
  }
  Option<Response> Network::httpRequest(const HttpMethod method, const Option<String> token, const String path, const Option<String> data) const {
    (void) method;
    (void) token;
    (void) path;
    (void) data;
    // TODO: is there a better way to mock this?
    return Option<Response>((Response) {
      .code = 200,
      .payload = "",
    });
  }
  String Network::macAddress() const { return "MAC::MAC::ERS::ON"; }
  void Network::disconnect() const {}
#endif