#include <network.hpp>
#include <flash.hpp>
#include <wps.hpp>
#include <log.hpp>

#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <configuration.h>

// TODO: remove this global state, it's needed so we don't constantly retry to connect with stale network credentials
// That we can't know it's stale because the router may be offline
// This will only happen if the network changes its name, a very rare event
// But not worth flooding the network
unsigned long lastTryStoredCredentials = 0;
const unsigned long intervalTryStoredCredentialsMillis = 60000; // 1 minute

void onConnection(const WiFiEventStationModeGotIP event) {
  logger.info("WiFi connected (" + event.ip.toString() + "): " + String(wifi_station_get_connect_status()));
  struct station_config config = {0};
  wifi_station_get_config(&config);

  WiFi.mode(WIFI_STA);

  const auto maybeCurrConfig = flash.readWifiConfig();
  if (maybeCurrConfig.isSome()) {
    lastTryStoredCredentials = 0;
    const auto currConfig = maybeCurrConfig.expect("Current network config missing when it shouldn't");
    const bool sameSsid = memcmp(currConfig.ssid, config.ssid, 32);
    const bool samePsk = memcmp(currConfig.password, config.password, 64);
    if (sameSsid && samePsk) {
      return;
    }
  }

  flash.writeWifiConfig(config);
  String msg = (char*) config.ssid;
  msg += ", " + String((char*) config.password);
  if (config.bssid_set) {
    msg += ", " + String((char*) config.bssid);
  }
  logger.info("Connected to: " + msg);
}

void Network::setup() const {
  #ifdef IOP_ONLINE
    static const auto handler = WiFi.onStationModeGotIP(onConnection);
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    WiFi.mode(WIFI_STA);
  #else
    WiFi.mode(WIFI_OFF);
  #endif

  auto status = wifi_station_get_connect_status();
  if (status != STATION_CONNECTING && status != STATION_GOT_IP) {
    this->connect();
  }
}

bool Network::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}


unsigned long lastLog = 0;
const unsigned long intervalLogMillis = 30000; // 10 seconds

bool Network::connect() const {
  const unsigned long now = millis();
  const bool log = lastLog == 0 || lastLog + intervalLogMillis < now;
  if (lastLog == 0 || lastLog + intervalLogMillis < now) {
    logger.info("Connect");
    lastLog = now;
  }

  #ifndef IOP_ONLINE
  return false;
  #endif

  #ifdef IOP_ONLINE
  if (lastTryStoredCredentials == 0
      || lastTryStoredCredentials + intervalTryStoredCredentialsMillis < now) {

    lastTryStoredCredentials = now;

    const auto maybeConfig = flash.readWifiConfig();
    if (maybeConfig.isSome()) {
      logger.info("Recovering last connection information");
      const auto config = maybeConfig.expect("Missing network config when it shouldn't");
      if (config.bssid_set) {
        WiFi.begin((char*) config.ssid, (char*) config.password, 0, config.bssid);
      } else {
        WiFi.begin((char*) config.ssid, (char*) config.password);
      }

      const auto success = WiFi.waitForConnectResult() != -1 && network.isConnected();
      const auto status = wifiCodeToString(WiFi.status());
      logger.debug("Wifi Status: " + status);
      if (success) {
        return true;
      } else {
        logger.error("Failed to connect to wifi. Status: " + status);
        const auto status = wifi_station_get_connect_status();
        // We don't flush the credentials if the network name is wrong
        // because a offline router will trigger that
        if (status == STATION_WRONG_PASSWORD) {
          flash.removeWifiConfig();
        }
      }
    }
  }

  if (wifiNetworkName.isNone() || wifiPassword.isNone()) {
    if (log) logger.trace("No wifi network name or password");
    return false;
  }

  const auto networkName = wifiNetworkName.expect("Missing network name when it shouldn't");
  const auto networkPassword = wifiPassword.expect("Missing network password when it shouldn't");
  WiFi.begin(networkName, networkPassword);

  const bool success = WiFi.waitForConnectResult();
  if (!success) {
    const auto status = wifiCodeToString(WiFi.status());
    logger.error("Failed to connect to wifi. Status: " + status);
  }
  return success;
  #endif
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

Option<Response> Network::httpRequest(const enum HttpMethod method, const Option<String> token, const String path, const Option<String> data) const {
  logger.info("[" + methodToString(method) + "] " + path + ": " + data.unwrap_or("No Data"));

  if (host.isNone()) {
    #ifdef IOP_MONITOR
    panic("No host available");
    #endif
    return Option<Response>();
  }

  #ifdef IOP_ONLINE
  #ifdef IOP_MONITOR
  for (uint8_t index = 0; index < 3; ++index) {
    if (isConnected()) {
      const auto uri = api.host() + path;

      WiFiClientSecure client;
      client.setInsecure(); // TODO: please fix it this is horrible
      client.connect(uri, 4001);

      HTTPClient http;
      http.begin(client, uri);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Connection", "close");
      if (token.isSome()) {
        http.addHeader("Authorization", "Basic " + token.expect("Missing auth token when it shouldn't"));
      }

      const auto payload = data.unwrap_or("");
      const int httpCode = http.sendRequest(methodToString(method).c_str(), (const uint8_t *) payload.c_str(), payload.length());

      logger.info("Response code: " + String(httpCode));
      if (httpCode == 200) {
        const auto payload = http.getString();
        http.end();

        return Option<Response>((Response) {
          .code = httpCode,
          .payload = Option<String>(payload),
        });
      } else {
        http.end();

        return Option<Response>((Response) {
          .code = httpCode,
          .payload = Option<String>(),
        });
      }
    } else {
      connect();
    }
  }
  #endif
  #endif

  return Option<Response>();
}
