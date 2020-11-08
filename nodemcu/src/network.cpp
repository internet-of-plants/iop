#include <ESP8266HTTPClient.h>

#include <network.hpp>

void Network::setup(std::function<void (const WiFiEventStationModeGotIP &)> onConnection) const {
  #ifdef IOP_ONLINE
    // Makes sure the event handler is never dropped and only set once
    static const auto handler = WiFi.onStationModeGotIP(onConnection);
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
    (void) onConnection;
    WiFi.mode(WIFI_OFF);
  #endif
}

bool Network::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

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
      panic("Unrecognized wifi status code: " + String(val));
  }
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
  panic("Invalid Method");
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

Option<Response> Network::httpRequest(const enum HttpMethod method, Option<String> token, const String path, Option<String> data) const {
  this->logger.info("[" + methodToString(method) + "] " + path + ": " + data.map<String>(clone).unwrap_or("<no data>"));

  #ifdef IOP_ONLINE
  #ifdef IOP_MONITOR
  for (uint8_t index = 0; index < 3; ++index) {
    if (this->isConnected()) {
      const auto uri = this->host_ + path;

      WiFiClientSecure client;
      client.setInsecure(); // TODO: please fix it this is horrible
      client.connect(uri, 4001);

      HTTPClient http;
      http.begin(client, uri);
      if (data.isSome()) {
        http.addHeader("Content-Type", "application/json");
      }
      http.addHeader("Connection", "close");
      if (token.isSome()) {
        http.addHeader("Authorization", "Basic " + token.expect("Missing auth token when it shouldn't"));
      }

      const auto data_ = data.unwrap_or("");
      const int httpCode = http.sendRequest(methodToString(method).c_str(), (uint8_t *) data_.c_str(), data_.length());

      this->logger.info("Response code: " + String(httpCode));
      if (httpCode < 0 || httpCode >= UINT16_MAX) {
        http.end();
        continue;
      }

      if (http.getSize() > 2048) {
        this->logger.error("Payload from server was too big: " + String(http.getSize()));
        continue;
      }

      const auto payload = http.getString();
      http.end();

      return Option<Response>((Response) {
        .code = (uint16_t) httpCode,
        .payload = payload,
      });
    } else {
      break;
    }
    yield();
  }
  #endif
  #endif

  return Option<Response>();
}
