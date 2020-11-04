#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <EEPROM.h>
#include <WiFiClient.h>

#include <cstdint>
#include <utils.hpp>
#include <flash.hpp>
#include <network.hpp>
#include <configuration.h>
#include <api.hpp>
#include <measurement.hpp>
#include <wps.hpp>

OneWire soilTemperatureOneWireBus(soilTemperaturePin);
DallasTemperature soilTemperatureSensor(&soilTemperatureOneWireBus);
DHT airTempAndHumiditySensor(airTempAndHumidityPin, dhtVersion);

// Honestly global state is garbage, but this is basically loop's state
unsigned long lastTime = 0;
unsigned long lastYieldLog = 0;
Option<WiFiServer> server = Option<WiFiServer>();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  EEPROM.begin(512);

  #ifdef IOP_SENSORS
    pinMode(soilResistivityPowerPin, OUTPUT);
    airTempAndHumiditySensor.begin();
    soilTemperatureSensor.begin();
  #endif

  Serial.begin(9600);
  Log().info("Setup");

  networkSetup();

  connect();
}

void loop() {
  handleInterrupt();
  
  const unsigned long currTime = millis();
  if (server.isSome()) {
    WiFiClient client = server.unwrap().available();
    if (client && client.available()) {
      const String request = client.readStringUntil('\n');
      if (request.startsWith("GET")) {
        const String response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<!DOCTYPE HTML>\r\n"
          "<html><body>\r\n"
          "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
          "  <h3>Please provide your IOP credentials, so we can get a authentication token to use</h3>\r\n"
          "  <form style='margin: 0 auto; width: 300px;' action='/' method='POST'>\r\n"
          "    <div><label for='username'>Username:</label><input name='username' type='text' /></div>\r\n"
          "    <div><label for='password'>Password:</label><input name='password' type='password' /></div>\r\n"
          "    <input type='submit' value='Submit' />"
          "  </form>"
          "</body></html>";
        client.println(response);
        client.flush();
        client.stop();
      } else if (request.startsWith("POST")) {
        String data;
        while (!(data = client.readStringUntil('\n')).isEmpty()) {
          Log().debug(data);
        }
        Option<String> username = Option<String>();
        Option<String> password = Option<String>();
        do {
          data = client.readStringUntil('&');
          if (data.startsWith("username=")) {
            data.replace("username=", "");
            username = Option<String>(data);
          } else if (data.startsWith("password=")) {
            data.replace("password=", "");
            password = Option<String>(data);
          }
        } while (!data.isEmpty());
        client.stop();

        if (username.isSome() && password.isSome()) {
          const Option<AuthToken> authToken = generateToken(username.unwrap(), password.unwrap());
          if (authToken.isSome()) {
            server.unwrap().close();
            server = Option<WiFiServer>();
            writeAuthTokenToEEPROM(authToken.unwrap());
          }
        }
      } else {
        client.stop();
      }
    }
  }
  
  if (readAuthTokenFromEEPROM().isNone() && iopEmail.isSome() && iopPassword.isSome()) {
    const Option<AuthToken> token = generateToken(iopEmail.unwrap(), iopPassword.unwrap());
    if (token.isSome()) {
      if (server.isSome()) {
        server.unwrap().close();
      }
      server = Option<WiFiServer>();
      writeAuthTokenToEEPROM(token.unwrap());
    }
  }
  
  const Option<AuthToken> authToken = readAuthTokenFromEEPROM();
  if (!isConnected() && !connect()) {
    // TODO: open a access point and provide a form for the ssid + password
  } else if (authToken.isNone()) {
    server = Option<WiFiServer>(80);
    server.unwrap().begin();
    server.unwrap().setNoDelay(false);
  } else if (authToken.isSome() && readPlantIdFromEEPROM().isNone()) {
    const String token = String((char*)authToken.unwrap().data());
    Option<PlantId> plantId = iopPlantId.map<PlantId>(stringToPlantId);

    if (plantId.isSome() && doWeOwnsThisPlant(token, String((char*)plantId.unwrap().data()))) {
      writePlantIdToEEPROM(plantId.unwrap());
    } else {
      plantId = getPlantId(token, String(WiFi.macAddress()));

      if (plantId.isSome()) {
        writePlantIdToEEPROM(plantId.unwrap());
      } else {
        Log().error("Unable to get plant id");
      }
    }
  } else if (lastTime == 0 || lastTime + interval < currTime) {
    lastTime = currTime;
    Log().info("Timer triggered");
    measureAndSend(airTempAndHumiditySensor, soilTemperatureSensor, soilResistivityPowerPin);
  } else if (lastYieldLog + 10000 < currTime) {    
    lastYieldLog = currTime;
    Log().debug("Waiting");
  }

  if (!isConnected()) {
    connect();
  }

  yield();
}