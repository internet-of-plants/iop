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

// TODO: Sometimes we wrap a String.c_str() in a StaticString. We only do that if 
// we know that the StaticString's lifetime is smaller than the one from the actual String
// but this is a dengerous game since generally a StaticString is assumed to be
// a string that lives forever: we should make a StringView for that

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
    // this->logger.info(STATIC_STRING("Memory:"), START, STATIC_STRING(" "));
    // this->logger.info(String(ESP.getFreeHeap()) + " " + String(ESP.getFreeContStack()) + " " + ESP.getFreeSketchSpace(), CONTINUITY);

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
      this->handlePlant(authToken.expect(STATIC_STRING("authToken none at loop()")));
    } else if (this->nextTime <= now) {
      const auto token = authToken.expect(STATIC_STRING("Calling sendEvent, authToken is None"));
      const auto id = plantId.expect(STATIC_STRING("Calling Sensors::measure, plantId is None"));
      this->handleMeasurements(token, id);
    } else if (this->nextYieldLog <= now) {
      this->nextYieldLog = now + 1000;
      this->logger.debug(STATIC_STRING("Waiting"));
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
  EventLoop(const StaticString host):
    sensors(soilResistivityPowerPin, soilTemperaturePin, airTempAndHumidityPin, dhtVersion),
    api(host, logLevel),
    credentialsServer(host, logLevel),
    logger(logLevel, STATIC_STRING("LOOP")),
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
          this->logger.info(STATIC_STRING("Resetting all user information saved in flash storage"));
          this->flash.removeWifiConfig();
          this->flash.removeAuthToken();
          this->flash.removePlantId();
          this->api.disconnect();
        #endif
        break;
      case ON_CONNECTION:
        #ifdef IOP_ONLINE
          this->logger.info(STATIC_STRING("WiFi connected ("), START, STATIC_STRING(" "));
          this->logger.info(WiFi.localIP().toString(), CONTINUITY, STATIC_STRING(" "));
          this->logger.info(STATIC_STRING("):"), CONTINUITY, STATIC_STRING(" "));
          this->logger.info(String(wifi_station_get_connect_status()), CONTINUITY);
          struct station_config config = {0};
          wifi_station_get_config(&config);

          WiFi.mode(WIFI_STA);

          const auto maybeCurrConfig = this->flash.readWifiConfig();
          if (maybeCurrConfig.isSome()) {
            const struct station_config & currConfig = maybeCurrConfig.asRef().expect(STATIC_STRING("Current network config missing when it shouldn't"));
            const bool sameSsid = memcmp(currConfig.ssid, config.ssid, 32);
            const bool samePsk = memcmp(currConfig.password, config.password, 64);
            if (sameSsid && samePsk) {
              break;
            }
          }

          this->flash.writeWifiConfig(config);
          this->logger.info(STATIC_STRING("Connected to:"), START, STATIC_STRING(" "));
          this->logger.info(StaticString((char*)config.ssid), CONTINUITY, STATIC_STRING(" "));
          this->logger.info(StaticString((char*)config.password), CONTINUITY);
        #endif
                  break;
      break;
    };
  }

  void handleCredentials(Option<AuthToken> maybeToken) {
    auto result = this->credentialsServer.serve(this->flash.readWifiConfig(), std::move(maybeToken));
    if (result.isOk()) {
      auto opt = result.expectOk(STATIC_STRING("Result is err but shouldn't: at loop()"));
      if (opt.isSome()) {
        this->flash.writeAuthToken(opt.expect(STATIC_STRING("AuthToken is None but shouldn't: at loop()")));
      }
    } else if (result.isErr()) {
      switch (result.expectErr(STATIC_STRING("Result is ok but shouldn't: at loop()"))) {
        case ServeError::REMOVE_WIFI_CONFIG:
          this->flash.removeWifiConfig();
          break;
      }
    } else {
      panic_(STATIC_STRING("Invalid result state"));
    }
  }

  void handlePlant(const AuthToken & token) const {
    auto maybePlantId = this->api.registerPlant(token);
    if (maybePlantId.isErr()) {
      this->logger.error(STATIC_STRING("Unable to get plant id"));
      auto maybeStatusCode = maybePlantId.expectErr(STATIC_STRING("maybePlantId is Ok but shouldn't"));
      if (maybeStatusCode.isSome() && maybeStatusCode.expect(STATIC_STRING("maybeStatusCode is None")) == 403) {
        this->flash.removeAuthToken();
      }
    } else if (maybePlantId.isOk()) {
      const auto plantId = maybePlantId.expectOk(STATIC_STRING("maybePlantId is Err but shouldn't"));
      this->flash.writePlantId(plantId);
    } else {
      panic_(STATIC_STRING("maybePlantId was empty when it shouldn't"));
    }
  }

  void handleMeasurements(const AuthToken & token, const PlantId & id) {
    this->nextTime = millis() + interval;
    this->logger.info(STATIC_STRING("Timer triggered"));

    digitalWrite(LED_BUILTIN, HIGH);
    auto maybeStatus = this->api.registerEvent(token, sensors.measure(id));

    if (maybeStatus.isNone()) {
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }

    const auto status = maybeStatus.expect(STATIC_STRING("Status is none"));
    if (status == 403) {
      this->logger.warn(STATIC_STRING("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
    } else if (status == 404) {
      this->logger.warn(STATIC_STRING("Plant Id was not found, deleting it"));
      this->flash.removePlantId();
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
};

static const char * const PROGMEM expected = "No host available";
std::unique_ptr<EventLoop> eventLoop = std::unique_ptr<EventLoop>(new EventLoop(host
  .map<StaticString>([](const StaticString & str) { return str; })
  .expect(StaticString(expected))));

void setup() {
  eventLoop->setup();
}

void loop() {
  eventLoop->loop();
  yield();
}
