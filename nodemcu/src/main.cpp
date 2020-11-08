#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <EEPROM.h>
#include <WiFiClient.h>
#include <Arduino.h>

#include <cstdint>
#include <flash.hpp>
#include <api.hpp>
#include <measurement.hpp>
#include <reset.hpp>
#include <server.hpp>
#include <sensors.hpp>
#include <log.hpp>
#include <utils.hpp>
#include <configuration.h>

class EventLoop {
private:
  Sensors sensors;
  Api api;
  CredentialsServer credentialsServer;
  Log logger;
  Flash flash;

  unsigned long nextTime;
  unsigned long nextYieldLog;
  
public:
  void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    reset::setup();
    this->logger.setup();
    this->sensors.setup();
    this->flash.setup();
    this->api.setup(this->makeConnectedCallback());
  }

  void loop() {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
      case NONE:
        break;
      case FACTORY_RESET:
        this->logger.info("Resetting all user information saved in flash storage");
        this->flash.removeWifiConfig();
        this->flash.removeAuthToken();
        this->flash.removePlantId();
        this->api.disconnect();
        yield();
        break;
    }

    const unsigned long now = millis();
    auto authToken = this->flash.readAuthToken();
    auto plantId = this->flash.readPlantId();

    if (this->api.isConnected() && authToken.isSome() && plantId.isSome()) {
      this->credentialsServer.close();
    }
    
    if (!this->api.isConnected() || authToken.isNone()) {
      auto result = this->credentialsServer.serve(this->flash.readWifiConfig(), std::move(authToken));
      if (result.isOk()) {
        auto opt = result.expectOk("Result is err but shouldn't: at loop()");
        if (opt.isSome()) {
          this->flash.writeAuthToken(opt.expect("AuthToken is None but shouldn't: at loop()"));
        }
      } else if (result.isErr()) {
        switch (result.expectErr("Result is ok but shouldn't: at loop()")) {
          case ServeError::REMOVE_WIFI_CONFIG:
            this->flash.removeWifiConfig();
            break;
        }
      } else {
        panic("Invalid result state");
      }
    } else if (plantId.isNone()) {
      const auto authToken_ = authToken.expect("authToken is None at loop");
      const String token = (char*) authToken_.data();
      auto maybePlantId = this->api.registerPlant(token);
      if (maybePlantId.isErr()) {
        logger.error("Unable to get plant id");
        auto maybeStatusCode = maybePlantId.expectErr("maybePlantId is Ok but shouldn't");
        if (maybeStatusCode.isSome() && maybeStatusCode.expect("maybeStatusCode is None") == 403) {
          this->flash.removeAuthToken();
        }
      } else if (maybePlantId.isOk()) {
        const auto plantId = maybePlantId.expectOk("maybePlantId is Err but shouldn't");
        this->flash.writePlantId(plantId);
      } else {
        panic("maybePlantId was empty when it shouldn't");
      }
    } else if (nextTime <= now) {
      nextTime = now + interval;
      logger.info("Timer triggered");
      
      digitalWrite(LED_BUILTIN, HIGH);
      const auto token = authToken.expect("Calling sendEvent, authToken is None");
      const auto id = plantId.expect("Calling Sensors::measure, plantId is None");
      auto maybeStatus = api.registerEvent(token, sensors.measure(id));

      if (maybeStatus.isNone()) {
        return;
      }
      const auto status = maybeStatus.expect("Status is none");
      if (status == 403) {
        logger.warn("Auth token was refused, deleting it");
        this->flash.removeAuthToken();
      } else if (status == 404) {
        logger.warn("Plant Id was not found, deleting it");
        this->flash.removePlantId();
      }
      digitalWrite(LED_BUILTIN, LOW);
    } else if (nextYieldLog <= now) {    
      nextYieldLog = now + 10000;
      logger.debug("Waiting");
    }
  }

  std::function<void (const WiFiEventStationModeGotIP &)> makeConnectedCallback() {
    return [this](const WiFiEventStationModeGotIP &event) {
      this->logger.info("WiFi connected (" + event.ip.toString() + "): " + String(wifi_station_get_connect_status()));
      struct station_config config = {0};
      wifi_station_get_config(&config);

      WiFi.mode(WIFI_STA);

      auto maybeCurrConfig = this->flash.readWifiConfig();
      if (maybeCurrConfig.isSome()) {
        const auto currConfig = maybeCurrConfig.expect("Current network config missing when it shouldn't");
        const bool sameSsid = memcmp(currConfig.ssid, config.ssid, 32);
        const bool samePsk = memcmp(currConfig.password, config.password, 64);
        if (sameSsid && samePsk) {
          return;
        }
      }

      this->flash.writeWifiConfig(config);
      String msg = (char*) config.ssid;
      msg += ", " + String((char*) config.password);
      if (config.bssid_set) {
        msg += ", " + String((char*) config.bssid);
      }
      this->logger.info("Connected to: " + msg);
    };
  }

  void operator=(EventLoop & other) = delete;
  void operator=(EventLoop && other) {
    this->sensors = std::move(other.sensors);
    this->api = std::move(other.api);
    this->logger = std::move(other.logger);
    this->flash = std::move(other.flash);
  }
  EventLoop(const String host):
    sensors(soilResistivityPowerPin, soilTemperaturePin, airTempAndHumidityPin, dhtVersion),
    api(host, logLevel),
    credentialsServer(host, logLevel),
    logger(logLevel, "LOOP") {}
};

EventLoop eventLoop(host.map<String>(clone).expect("Host cant be none"));

void setup() {
  eventLoop.setup();
}

void loop() {
  eventLoop.loop();
  yield();
}