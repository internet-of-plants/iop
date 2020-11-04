#include <network.hpp>
#include <flash.hpp>
#include <wps.hpp>

#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <configuration.h>

void onConnection(const WiFiEventStationModeConnected event) {
  struct station_config config = {0};
  wifi_station_get_config(&config);

  const Option<struct station_config> currConfig = readWifiConfigFromEEPROM();
  if (currConfig.isSome()) {
    const bool sameSsid = memcmp(currConfig.unwrap().ssid, config.ssid, 32);
    const bool samePsk = memcmp(currConfig.unwrap().password, config.password, 64);
    if (sameSsid && samePsk) {
      return;
    }
  }

  writeWifiConfigToEEPROM(config);
  String msg = (char*) config.ssid;
  msg += ", " + String((char*) config.password);
  msg += ", " + String(config.bssid_set);
  msg += ", " + String((char*) config.bssid);
  Log().info("Connected to: " + msg);
}

void networkSetup() {
  EEPROM.begin(512);
  attachInterrupt(digitalPinToInterrupt(wpsButton), wpsButtonUp, RISING);

  #ifdef IOP_ONLINE
    WiFi.mode(WIFI_STA);
    WiFi.onStationModeConnected(onConnection);
  #else
    WiFi.mode(WIFI_OFF);
  #endif
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

bool waitForConnection() {
  unsigned long startTime = millis();
  unsigned long lastPrint = 0;
  unsigned long lastLedToggle = 0;
  unsigned long now = startTime;
  bool ledOn = false;

  while (!isConnected() && startTime + 30000 < (now = millis())) {
    if (lastPrint + 5000 < now) {
      Log().debug("Connecting...");
      lastPrint = now;
    }
    if (lastLedToggle + 500 < now) {
      ledOn = !ledOn;
      if (ledOn) {
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        digitalWrite(LED_BUILTIN, LOW);
      }
      lastLedToggle = now;
    }
    yield();
  }
  if (ledOn) {
    digitalWrite(LED_BUILTIN, LOW);
  }
  return isConnected();
}

bool connect() {
  Log().info("Connect");
  
  #ifndef IOP_ONLINE
  return false; // TODO: should we mock it and return true? If we mock, how can we mock actual requests?
  #endif

  #ifdef IOP_ONLINE
  const Option<struct station_config> maybeConfig = readWifiConfigFromEEPROM();
  if (maybeConfig.isSome()) {
    Log().info("Recovering last connection information");
    const station_config config = maybeConfig.unwrap();
    if (config.bssid_set) {
      WiFi.begin((char*) config.ssid, (char*) config.password, 0, config.bssid);
    } else {
      WiFi.begin((char*) config.ssid, (char*) config.password);
    }

    const bool success = waitForConnection();
    const String status = wifiCodeToString(WiFi.status());
    Log().debug("Wifi Status: " + status);
    if (success) {
      return true;
    } else {
      Log().error("Failed to connect to wifi. Status: " + status);
      // To delete it from EEPROM we should make sure the error was invalid credentials
      // And not a offline router or w/e
      // And even then maybe we shouldn't delete it at all.
      //
      // TODO: If connection fails we should open a access point with a form to submit the credential
      // Or wait for a WPS. But the information stored doesn't need to be deleted.
      // To do that we need to retry the EEPROM saved credentials from time to time.
      // While we have the access point open. And when the credentials are submitted we rewrite it
      //removeWifiConfigFromEEPROM();
    }
  }

  if (wifiNetworkName.isNone() || wifiPassword.isNone()) {
    Log().debug("No wifi network name or password");
    // TODO: setup own wifi network so we connect to id and pass the credentials through it
    return false;
  }

  WiFi.begin(wifiNetworkName.unwrap(), wifiPassword.unwrap());

  const bool success = waitForConnection();
  const String status = wifiCodeToString(WiFi.status());
  Log().debug("Wifi Status: " + status);
  if (!success) {
    Log().error("Failed to connect to wifi. Status: " + status);
  }
  return success;
  #endif
}

String wifiCodeToString(const wl_status_t val) {
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

Option<Response> httpPut(const String token, const String path, const String data) {
  return httpRequest(PUT, Option<String>(token), path, Option<String>(data));
}

Option<Response> httpPost(const String token, const String path, const String data) {
  return httpRequest(POST, Option<String>(token), path, Option<String>(data));
}

Option<Response> httpPost(const String path, const String data) {
  return httpRequest(POST, Option<String>(), path, Option<String>(data));
}

Option<Response> httpRequest(const enum HttpMethod method, const Option<String> token, const String path, const Option<String> data) {
  Log().info("[" + methodToString(method) + "] " + path + ": " + data.unwrap_or("No Data"));

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
      const String uri = host.unwrap() + path;

      WiFiClientSecure client;
      client.setInsecure(); // TODO: please fix it this is horrible
      client.connect(uri, 4001);

      HTTPClient http;
      http.begin(client, uri);
      http.addHeader("Content-Type", "application/json");
      if (token.isSome()) {
        http.addHeader("Authorization", "Basic " + token.unwrap());
      }

      const String payload = data.unwrap();
      const int httpCode = http.sendRequest(methodToString(method).c_str(), (const uint8_t *) payload.c_str(), payload.length());

      Log().info("Response code: " + String(httpCode));
      if (httpCode == 200) {
        const String payload = http.getString();
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