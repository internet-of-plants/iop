#include "credentials.h"
#include "models.h"

#include <cstdint>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>

const String host = "https://iop-monitor-server.tk:4001";

unsigned long lastTime = 0;
const uint32_t timerDelay = 60 * 1000;

void sendEvent(const String token, const Event event);
String generateToken();
void connect();

// Isn't there a Option struct lying around somewhere?
String token = "";

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(100);
  Serial.println("Bro");
  connect();
}

void loop() {
  const unsigned long currTime = millis();
  if ((currTime > 0 && lastTime == 0) || (currTime - lastTime) > timerDelay) {
    Serial.println("Timer triggered ");

    if (token.length() == 0 && (token = generateToken()).length() == 0) {
      Serial.println("Token is empty");
      lastTime = currTime;
      return;
    }
    Event event = {};
    event.airTemperatureCelsius = 25;
    event.airHumidityPercentage = 30;
    event.soilResistivityRaw = 800;
    event.soilTemperatureCelsius = 20;
    event.plantId = iopPlantId;

    sendEvent(token, event);
    lastTime = currTime;
  } else if (lastTime != 0 && lastTime + timerDelay > currTime) {
    delay(lastTime + timerDelay - currTime);
  } else {
    delay(500);
  }
}

void connect() {
  Serial.println("Connect");
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.begin(wifiNetworkName, wifiPassword);
  uint8_t count = 0;
  while (WiFi.status() != WL_CONNECTED && count++ < 120) {
   delay(500);
   Serial.println("Connecting...");
  }
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("Connected? ");
  Serial.print(WiFi.status());
}

void sendEvent(const String token, const Event event) {
  if (token.length() == 0) {
    Serial.println("Error: sendEvent's token was empty");
    return;
  }

  Serial.println("Send event " + token);
  if (WiFi.status() == WL_CONNECTED) {
    const String uri = host + "/event";

    StaticJsonDocument<5024> doc;
    doc["air_temperature_celsius"] = event.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.airHumidityPercentage;
    doc["soil_temperature_celsius"] = event.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.soilResistivityRaw;
    doc["plant_id"] = event.plantId;

    char buffer[5024];
    serializeJson(doc, buffer);

    WiFiClientSecure client;
    client.setInsecure(); // TODO: pwease fix it this is horrible
    client.connect(uri, 4001);

    HTTPClient http;
    http.begin(client, uri);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Basic " + token);

    const int httpCode = http.POST(buffer);

    Serial.print("Response code was: ");
    Serial.println(httpCode);
    if (httpCode != 200) {
      for (uint8_t count = 0; count < 30; count++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(300);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    http.end();
  } else {
    connect();
  }
}

String generateToken() {
  Serial.print("Generating token ");
  Serial.println(WiFi.status());
  if (WiFi.status() == WL_CONNECTED) {
    const String uri = host + "/user/login";

    StaticJsonDocument<300> doc;

    doc["email"] = iopEmail;
    doc["password"] = iopPassword;

    char buffer[300];
    serializeJson(doc, buffer);

    WiFiClientSecure client;
    client.setInsecure(); // TODO: pwease fix it this is horrible
    client.connect(uri, 4001);

    HTTPClient http;
    http.begin(client, uri);
    http.addHeader("Content-Type", "application/json");

    const int httpCode = http.POST(buffer);

    Serial.print("Response code was: ");
    Serial.println(httpCode);
    if (httpCode == 200) {
      const String payload = http.getString();
      http.end();
      return payload;
    } else {
      for (uint8_t count = 0; count < 30; count++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(300);
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    http.end();
  } else {
    connect();
  }
  return "";
}
