#include <utils.hpp>
#include <flash.hpp>
#include <api.hpp>
#include <reset.hpp>
#include <server.hpp>
#include <sensors.hpp>
#include <log.hpp>
#include <configuration.h>
#include <static_string.hpp>

// TODO: https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/timeSync.cpp
// TODO: Over the Air updates (OTA)

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
    this->api.setup();
  }

  void loop() noexcept {
    #ifdef LOG_MEMORY
    this->logger.info(F("Memory:"), START, F(" "));
    this->logger.info(String(ESP.getFreeHeap()) + " " + String(ESP.getFreeContStack()) + " " + ESP.getFreeSketchSpace(), CONTINUITY);
    #endif

    this->handleInterrupt();

    const unsigned long now = millis();
    auto authToken = this->flash.readAuthToken();
    auto plantId = this->flash.readPlantId();

    if (this->api.isConnected() && authToken.isSome() && plantId.isSome()) {
      this->credentialsServer.close();
    }

    if (!this->api.isConnected() || authToken.isNone()) {
      this->handleCredentials(std::move(authToken));
    } else if (plantId.isNone()) {
      this->handlePlant(authToken.expect(F("authToken none at loop()")));
    } else if (this->nextTime <= now) {
      const auto token = authToken.expect(F("Calling sendEvent, authToken is None"));
      const auto id = plantId.expect(F("Calling Sensors::measure, plantId is None"));
      this->handleMeasurements(token, id);
    } else if (this->nextYieldLog <= now) {
      this->nextYieldLog = now + 1000;
      this->logger.debug(F("Waiting"));
    }
  }

  void operator=(EventLoop & other) = delete;
  void operator=(EventLoop && other) {
    this->sensors = std::move(other.sensors);
    this->api = std::move(other.api);
    this->logger = std::move(other.logger);
    this->credentialsServer = std::move(other.credentialsServer);
    this->flash = std::move(other.flash);
  }
  EventLoop(const StringView host):
    sensors(soilResistivityPowerPin, soilTemperaturePin, airTempAndHumidityPin, dhtVersion),
    api(host, logLevel),
    credentialsServer(host, logLevel),
    logger(logLevel, F("LOOP")),
    flash(logLevel) {}

private:
  void handleInterrupt() const {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
      case NONE:
        break;
      case FACTORY_RESET:
        #ifdef IOP_FACTORY_RESET
          this->logger.info(F("Resetting all user information saved in flash storage"));
          this->flash.removeWifiConfig();
          this->flash.removeAuthToken();
          this->flash.removePlantId();
          this->api.disconnect();
        #endif
        break;
      case ON_CONNECTION:
        #ifdef IOP_ONLINE
          this->logger.info(F("WiFi connected ("), START, F(" "));
          this->logger.info(WiFi.localIP().toString(), CONTINUITY, F(" "));
          this->logger.info(F("):"), CONTINUITY, F(" "));
          this->logger.info(String(wifi_station_get_connect_status()), CONTINUITY);
          struct station_config config = {0};
          wifi_station_get_config(&config);

          WiFi.mode(WIFI_STA);

          NetworkName ssid((NetworkName::Storage) {0});
          memcpy(ssid.mutPtr(), config.ssid, NetworkName::dataSize);
          NetworkPassword psk((NetworkPassword::Storage) {0});
          memcpy(psk.mutPtr(), config.password, NetworkPassword::dataSize);

          auto maybeCurrConfig = this->flash.readWifiConfig();
          if (maybeCurrConfig.isSome()) {
            auto currConfig = maybeCurrConfig.expect(F("Current network config missing when it shouldn't"));
            const bool sameSsid = memcmp(currConfig.ssid.constPtr(), (uint8_t*) config.ssid, NetworkName::dataSize);
            const bool samePsk = memcmp(currConfig.password.constPtr(), (uint8_t*) config.password, NetworkPassword::dataSize);
            if (sameSsid && samePsk) {
              break;
            }

            this->flash.writeWifiConfig((struct WifiCredentials) { .ssid = ssid, .password = psk });
          }
          this->logger.info(F("Connected to:"), START, F(" "));
          this->logger.info(ssid.asString(), CONTINUITY);
        #endif
      break;
    };
  }

  void handleCredentials(Option<AuthToken> maybeToken) {
    auto result = this->credentialsServer.serve(this->flash.readWifiConfig(), maybeToken);
    if (result.isOk()) {
      auto opt = result.expectOk(F("Result is err but shouldn't: at loop()"));
      if (opt.isSome()) {
        this->flash.writeAuthToken(opt.expect(F("AuthToken is None but shouldn't: at loop()")));
      }
    } else if (result.isErr()) {
      switch (result.expectErr(F("Result is ok but shouldn't: at loop()"))) {
        case ServeError::REMOVE_WIFI_CONFIG:
          this->flash.removeWifiConfig();
          break;
      }
    } else {
      panic_(F("Invalid result state"));
    }
  }

  void handlePlant(const AuthToken & token) const {
    auto maybePlantId = this->api.registerPlant(token);
    if (maybePlantId.isErr()) {
      this->logger.error(F("Unable to get plant id"));
      auto maybeStatusCode = maybePlantId.expectErr(F("maybePlantId is Ok but shouldn't"));
      if (maybeStatusCode.isSome() && maybeStatusCode.expect(F("maybeStatusCode is None")) == 403) {
        this->flash.removeAuthToken();
      }
    } else if (maybePlantId.isOk()) {
      const auto plantId = maybePlantId.expectOk(F("maybePlantId is Err but shouldn't"));
      this->flash.writePlantId(plantId);
    } else {
      panic_(F("maybePlantId was empty when it shouldn't"));
    }
  }

  void handleMeasurements(const AuthToken & token, const PlantId & id) {
    this->nextTime = millis() + interval;
    this->logger.info(F("Timer triggered"));

    digitalWrite(LED_BUILTIN, HIGH);
    auto maybeStatus = this->api.registerEvent(token, sensors.measure(id));

    if (maybeStatus.isNone()) {
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }

    const auto status = maybeStatus.expect(F("Status is none"));
    if (status == 403) {
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
    } else if (status == 404) {
      this->logger.warn(F("Plant Id was not found, deleting it"));
      this->flash.removePlantId();
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
};

static const char * const PROGMEM expected = "No host available";
std::unique_ptr<EventLoop> eventLoop = std::unique_ptr<EventLoop>(new EventLoop(host
  .map<StringView>([](const StringView & str) { return str; })
  .expect(StringView(expected))));

void setup() {
  eventLoop->setup();
}

void loop() {
  eventLoop->loop();
  yield();
}