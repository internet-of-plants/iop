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
#include <server.hpp>
#include <sensors.hpp>
#include <log.hpp>

Sensors sensors = Sensors();
MonitorCredentialsServer monitorCredentialsServer = MonitorCredentialsServer();

// Honestly global state is garbage, but this is basically loop's state
unsigned long lastTime = 0;
unsigned long lastYieldLog = 0;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  logger.setup();
  sensors.setup();
  flash.setup();
  network.setup();
  api.setup();
}

void loop() {
  handleInterrupt();

  const unsigned long currTime = millis();
  const Option<AuthToken> authToken = flash.readAuthToken();
  const Option<PlantId> plantId = flash.readPlantId();

  if (!network.isConnected() && !network.connect()) {
    // TODO: open a access point and provide a form for the ssid + password
  } else if (authToken.isNone()) {
    monitorCredentialsServer.serve();
  } else if (authToken.isSome() && plantId.isNone()) {
    const AuthToken token = authToken.expect("Calling handlePlantId, authToken is None");
    handlePlantId(token, WiFi.macAddress());
  } else if (lastTime == 0 || lastTime + interval < currTime) {
    digitalWrite(LED_BUILTIN, HIGH);

    lastTime = currTime;
    logger.info("Timer triggered");
    
    const AuthToken token = authToken.expect("Calling sendEvent, authToken is None");
    const PlantId id = plantId.expect("Calling Sensors::measure, plantId is None");
    api.registerEvent(token, sensors.measure(id));

    digitalWrite(LED_BUILTIN, LOW);
  } else if (lastYieldLog + 10000 < currTime) {    
    lastYieldLog = currTime;
    logger.debug("Waiting");
  }

  yield();
}