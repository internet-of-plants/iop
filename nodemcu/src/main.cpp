#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"
#include "log.hpp"
#include "reset.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "static_string.hpp"
#include "utils.hpp"

// TODO:
// https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/timeSync.cpp
// TODO: Over the Air updates (OTA) using ESPhttpUpdate

class EventLoop {
private:
  Sensors sensors;
  Api api;
  CredentialsServer credentialsServer;
  Log logger;
  Flash flash;
  MD5Hash firmwareHash;

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
    this->logger.infoln(F("Memory: "), std::to_string(ESP.getFreeHeap()),
                        F(" "), std::to_string(ESP.getFreeContStack()), F(" "),
                        ESP.getFreeSketchSpace());
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
      this->handlePlant(UNWRAP_REF(authToken));

    } else if (this->nextTime <= now) {
      this->handleMeasurements(UNWRAP_REF(authToken), UNWRAP_REF(plantId));

    } else if (this->nextYieldLog <= now) {
      this->nextYieldLog = now + 1000;
      this->logger.debugln(F("Waiting"));
    }
  }

  EventLoop &operator=(EventLoop &other) = delete;
  EventLoop &operator=(EventLoop &&other) = delete;
  EventLoop(const StaticString host) noexcept
      : sensors(soilResistivityPowerPin, soilTemperaturePin,
                airTempAndHumidityPin, dhtVersion),
        api(host, logLevel), credentialsServer(host, logLevel),
        logger(logLevel, F("LOOP")), flash(logLevel),
        firmwareHash(hashSketch()) {}

private:
  void handleInterrupt() const noexcept {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
    case NONE:
      break;
    case MUST_UPGRADE:
#ifdef IOP_OTA
      const auto maybeToken = this->flash.readAuthToken();
      if (maybeToken.isSome()) {
        const auto &token = UNWRAP_REF(maybeToken);
        const auto maybeCode = this->api.upgrade(token, this->firmwareHash);
        // TODO: handle maybeCode
      } else {
        // TODO: this should never happen, how to ensure?
      }
#endif
      break;
    case FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.infoln(F("Factory Reset: deleting stored credentials"));
      this->flash.removeWifiConfig();
      this->flash.removeAuthToken();
      this->flash.removePlantId();
      this->api.disconnect();
#endif
      break;
    case ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto ip = WiFi.localIP().toString();
      const auto status = std::to_string(wifi_station_get_connect_status());
      this->logger.infoln(F("WiFi connected ("), ip, F("): "), status);

      struct station_config config = {0};
      wifi_station_get_config(&config);

      const auto rawSsid = UnsafeRawString((char *)config.ssid);
      const auto ssid = NetworkName::fromStringTruncating(rawSsid);
      this->logger.infoln(F("Connected to network:"), ssid.asString());

      const auto rawPsk = UnsafeRawString((char *)config.password);
      const auto psk = NetworkPassword::fromStringTruncating(rawPsk);

      const auto maybeCurrConfig = this->flash.readWifiConfig();
      if (maybeCurrConfig.isSome()) {
        const auto &currConfig = UNWRAP_REF(maybeCurrConfig);

        if (currConfig.ssid.asString() == ssid.asString() &&
            currConfig.password.asString() == psk.asString()) {
          // No need to save credential that already are stored
          break;
        }
      }

      this->flash.writeWifiConfig({.ssid = ssid, .password = psk});
#endif
      break;
    };
  }

  void handleCredentials(const Option<AuthToken> &maybeToken) noexcept {
    const auto wifiConfig = this->flash.readWifiConfig();
    const auto result = this->credentialsServer.serve(wifiConfig, maybeToken);

    if (IS_ERR(result)) {
      switch (UNWRAP_ERR_REF(result)) {
      case ServeError::INVALID_WIFI_CONFIG:
        this->flash.removeWifiConfig();
        break;
      }

    } else {
      const auto &opt = UNWRAP_OK_REF(result);

      if (opt.isSome()) {
        this->flash.writeAuthToken(UNWRAP_REF(opt));
      }
    }
  }

  void handlePlant(const AuthToken &token) const noexcept {
    const auto maybePlantId = this->api.registerPlant(token);

    if (IS_ERR(maybePlantId)) {
      this->logger.errorln(F("Unable to get plant id"));

      const auto &maybeStatusCode = UNWRAP_ERR_REF(maybePlantId);

      if (maybeStatusCode.isSome()) {
        const auto &code = UNWRAP_REF(maybeStatusCode);

        // Forbidden (403): our auth token is invalid
        // Not found (404): our plant id is invalid (shouldn't happen)
        // Bad request (400): the json didn't fit the local buffer, this bad
        if (code == 403) {
          this->flash.removeAuthToken();
        } else if (code == 404) {
          this->flash.removePlantId();
        } else if (code == 400) {
          // TODO: handle this (how tho?)
        }
      }
    } else if (IS_OK(maybePlantId)) {
      this->flash.writePlantId(UNWRAP_OK_REF(maybePlantId));
    }
  }

  void handleMeasurements(const AuthToken &token, const PlantId &id) noexcept {
    this->nextTime = millis() + interval;
    this->logger.debugln(F("Handle Measurements"));

    digitalWrite(LED_BUILTIN, HIGH);
    const auto measurements = sensors.measure(id, this->firmwareHash);
    const auto maybeStatus = this->api.registerEvent(token, measurements);

    if (maybeStatus.isNone()) {
      digitalWrite(LED_BUILTIN, LOW);
      return;
    }

    const auto &status = UNWRAP_REF(maybeStatus);
    if (status == 403) {
      this->logger.warnln(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
    } else if (status == 404) {
      this->logger.warnln(F("Plant Id was not found, deleting it"));
      this->flash.removePlantId();
    } else if (status == 412) { // Must upgrade binary
      interruptEvent = MUST_UPGRADE;
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
};

PROGMEM_STRING(missingHost, "No host available");
auto eventLoop = make_unique<EventLoop>(host.asRef().expect(missingHost));

void setup() noexcept { eventLoop->setup(); }
void loop() noexcept { eventLoop->loop(); }