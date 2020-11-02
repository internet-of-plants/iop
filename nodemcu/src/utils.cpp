#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <configuration.h>
#include <utils.hpp>

void panic(const String msg) {
    Serial.println(msg);
    __panic_func(PSTR(__FILE__), __LINE__, __func__);
}

bool connect() {
  Serial.println("Connect");
  digitalWrite(LED_BUILTIN, HIGH);

  #ifndef IOP_ONLINE
  return false; // TODO: should we mock it and return true? If we mock, how can we mock actual requests?
  #endif

  #ifdef IOP_ONLINE
  WiFi.begin(wifiNetworkName, wifiPassword);
  uint8_t count = 0;
  while (WiFi.status() != WL_CONNECTED && count++ < 120) {
    Serial.println("Connecting...");
    delay(500);
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Connected? ");
  const bool success = WiFi.status() == WL_CONNECTED;
  Serial.print(success);
  Serial.print(" ");
  Serial.println(WiFi.status());
  return success;
  #endif
}

Option<Response> httpPost(const String token, const String path, const String data) {
  return httpPost(Option<String>(token), path, data);
}

Option<Response> httpPost(const String path, const String data) {
  return httpPost(Option<String>(), path, data);
}

Option<Response> httpPost(const Option<String> token, const String path, const String data) {
  Serial.println(WiFi.status());
  Serial.println(path + ": " + data);

  #ifdef ONLINE
  for (uint8_t index = 0; index < 3; ++index) {
    if (WiFi.status() == WL_CONNECTED) {
      const String uri = host + path;

      WiFiClientSecure client;
      client.setInsecure(); // TODO: please fix it this is horrible
      client.connect(uri, 4001);

      HTTPClient http;
      http.begin(client, uri);
      http.addHeader("Content-Type", "application/json");
      if (token.isSome()) {
        http.addHeader("Authorization", "Basic " + token.unwrap());
      }

      const int httpCode = http.POST(data);

      Serial.print("Response code was: ");
      Serial.println(httpCode);
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

  return Option<Response>();
}