#include <api.hpp>
#include <configuration.h>
#include <flash.hpp>
#include <log.hpp>
#include <reset.hpp>
#include <sensors.hpp>
#include <server.hpp>
#include <static_string.hpp>
#include <utils.hpp>

// TODO:
// https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/timeSync.cpp
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
  void setup() noexcept {
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
    this->logger.info(String(ESP.getFreeHeap()) + " " +
                          String(ESP.getFreeContStack()) + " " +
                          ESP.getFreeSketchSpace(),
                      CONTINUITY);
#endif

    this->handleInterrupt();

    const unsigned long now = millis();
    const auto authToken = this->flash.readAuthToken();
    const auto plantId = this->flash.readPlantId();

    if (this->api.isConnected() && authToken.isSome() && plantId.isSome()) {
      this->credentialsServer.close();
    }

    if (!this->api.isConnected() || authToken.isNone()) {
      this->handleCredentials(authToken);
    } else if (plantId.isNone()) {
      this->handlePlant(
          authToken.asRef().expect(F("authToken none at loop()")));
    } else if (this->nextTime <= now) {
      const AuthToken &token =
          authToken.asRef().expect(F("Calling sendEvent, authToken is None"));
      const PlantId &id = plantId.asRef().expect(
          F("Calling Sensors::measure, plantId is None"));
      this->handleMeasurements(token, id);
    } else if (this->nextYieldLog <= now) {
      this->nextYieldLog = now + 1000;
      this->logger.debug(F("Waiting"));
    }
  }

  EventLoop &operator=(EventLoop &other) = delete;
  EventLoop &operator=(EventLoop &&other) = delete;
  EventLoop(const StaticString host) noexcept
      : sensors(soilResistivityPowerPin, soilTemperaturePin,
                airTempAndHumidityPin, dhtVersion),
        api(host, logLevel), credentialsServer(host, logLevel),
        logger(logLevel, F("LOOP")), flash(logLevel) {}

private:
  void handleInterrupt() const noexcept {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
    case NONE:
      break;
    case FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.info(
          F("Resetting all user information saved in flash storage"));
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

      const auto ssid = NetworkName::fromStringTruncating(
          UnsafeRawString((char *)config.ssid));
      const auto psk = NetworkPassword::fromStringTruncating(
          UnsafeRawString((char *)config.password));

      const auto maybeCurrConfig = this->flash.readWifiConfig();
      if (maybeCurrConfig.isSome()) {
        const WifiCredentials &currConfig =
            maybeCurrConfig.asRef().expect(F("maybeCurrConfig is none"));
        if (strcmp(currConfig.ssid.asString().get(), ssid.asString().get()) &&
            strcmp(currConfig.password.asString().get(),
                   psk.asString().get())) {
          break;
        }

        this->flash.writeWifiConfig(
            (struct WifiCredentials){.ssid = ssid, .password = psk});
      }
      this->logger.info(F("Connected to:"), START, F(" "));
      this->logger.info(ssid.asString(), CONTINUITY);
#endif
      break;
    };
  }

  void handleCredentials(const Option<AuthToken> &maybeToken) noexcept {
    const auto result =
        this->credentialsServer.serve(this->flash.readWifiConfig(), maybeToken);
    if (result.isErr()) {
      const ServeError &err =
          result.asRef().expectErr(F("Result is ok but shouldn't: at loop()"));
      switch (err) {
      case ServeError::INVALID_WIFI_CONFIG:
        this->flash.removeWifiConfig();
        break;
      }
    } else if (result.isOk()) {
      const Option<AuthToken> &opt =
          result.asRef().expectOk(F("Result is err but shouldn't: at loop()"));
      if (opt.isSome()) {
        this->flash.writeAuthToken(opt.asRef().expect(
            F("AuthToken is None but shouldn't: at loop()")));
      }
    }
  }

  void handlePlant(const AuthToken &token) const noexcept {
    const auto maybePlantId = this->api.registerPlant(token);
    if (maybePlantId.isErr()) {
      this->logger.error(F("Unable to get plant id"));
      const Option<HttpCode> &maybeStatusCode =
          maybePlantId.asRef().expectErr(F("maybePlantId is Ok but shouldn't"));
      if (maybeStatusCode.isSome()) {
        const HttpCode &code =
            maybeStatusCode.asRef().expect(F("maybeStatusCode is None"));
        if (code == 403) {
          this->flash.removeAuthToken();
        }
      }
    } else if (maybePlantId.isOk()) {
      const PlantId &plantId =
          maybePlantId.asRef().expectOk(F("maybePlantId is Err but shouldn't"));
      this->flash.writePlantId(plantId);
    }
  }

  void handleMeasurements(const AuthToken &token, const PlantId &id) noexcept {
    this->nextTime = millis() + interval;
    this->logger.info(F("Timer triggered"));

    digitalWrite(LED_BUILTIN, HIGH);
    const auto maybeStatus =
        this->api.registerEvent(token, sensors.measure(id));

    if (maybeStatus.isNone()) {
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }

    const HttpCode &status = maybeStatus.asRef().expect(F("Status is none"));
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

PROGMEM_STRING(expected, "No host available");
std::unique_ptr<EventLoop> eventLoop =
    std::unique_ptr<EventLoop>(new EventLoop(host.asRef().expect(expected)));

void setup() noexcept { eventLoop->setup(); }

void loop() noexcept {
  eventLoop->loop();
  yield();
}