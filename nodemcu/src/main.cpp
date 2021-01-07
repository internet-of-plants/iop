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
      this->logger.debug(F("Waiting"));
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
        const auto status = this->api.upgrade(token, this->firmwareHash);
        switch (status) {
        case ApiStatus::FORBIDDEN:
          this->logger.warn(F("Invalid auth token, but keeping since at OTA"));
          return;

        case ApiStatus::NOT_FOUND:
          this->logger.warn(F("Invalid plant id, but keeping since at OTA"));
          return;

        case ApiStatus::BROKEN_SERVER:
          // Central server is broken. Nothing we can do besides waiting
          return;

        case ApiStatus::CLIENT_BUFFER_OVERFLOW:
          panic_(F("Api::upgrade internal buffer overflow"));

        case ApiStatus::MUST_UPGRADE: // Bruh
        case ApiStatus::BROKEN_PIPE:
        case ApiStatus::TIMEOUT:
        case ApiStatus::NO_CONNECTION:
          // Nothing to be done besides retrying later
          return;

        case ApiStatus::OK: // Cool beans
          return;
        }

        const auto str = this->api.network().apiStatusToString(status);
        this->logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
      } else {
        // TODO: this should never happen, how to ensure?
      }
#endif
      break;
    case FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.info(F("Factory Reset: deleting stored credentials"));
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
      this->logger.info(F("WiFi connected ("), ip, F("): "), status);

      struct station_config config = {0};
      wifi_station_get_config(&config);

      const auto rawSsid = UnsafeRawString((char *)config.ssid);
      const auto ssid = NetworkName::fromStringTruncating(rawSsid);
      this->logger.info(F("Connected to network:"), ssid.asString());

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
      this->logger.error(F("Unable to get plant id"));

      const auto &status = UNWRAP_ERR_REF(maybePlantId);
      switch (status) {
      case ApiStatus::FORBIDDEN:
        this->logger.warn(F("Auth token was refused, deleting it"));
        this->flash.removeAuthToken();
        return;

      case ApiStatus::CLIENT_BUFFER_OVERFLOW:
        panic_(F("Api::registerPlant internal buffer overflow"));
        return;

      case ApiStatus::BROKEN_SERVER:
      case ApiStatus::NOT_FOUND:
        // Server is broken. Nothing we can do besides waiting
        this->logger.warn(F("EventLoop::handlePlant server error"));

        ESP.deepSleep(5 * 60 * 1000 * 1000);
        return;

      case ApiStatus::BROKEN_PIPE:
      case ApiStatus::TIMEOUT:
      case ApiStatus::NO_CONNECTION:
        // Nothing to be done besides retrying later
        delay(5000);
        return;

      case ApiStatus::OK: // Cool beans
        return;

      case ApiStatus::MUST_UPGRADE:
        interruptEvent = InterruptEvent::MUST_UPGRADE;
        return;
      }

      const auto s = this->api.network().apiStatusToString(status);
      this->logger.error(F("Unexpected status, EventLoop::handlePlant: "), s);

    } else {
      this->flash.writePlantId(UNWRAP_OK_REF(maybePlantId));
    }
  }

  void handleMeasurements(const AuthToken &token, const PlantId &id) noexcept {
    this->nextTime = millis() + interval;
    this->logger.debug(F("Handle Measurements"));

    const auto measurements = sensors.measure(id, this->firmwareHash);
    const auto status = this->api.registerEvent(token, measurements);

    switch (status) {
    case ApiStatus::FORBIDDEN:
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
      return;

    case ApiStatus::NOT_FOUND:
      this->logger.warn(F("Plant Id was not found, deleting it"));
      this->flash.removePlantId();
      return;

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      panic_(F("Api::registerEvent internal buffer overflow"));

    case ApiStatus::BROKEN_SERVER:
      // Central server is broken. Nothing we can do besides waiting
      // It doesn't make much sense to log events to flash
      return;

    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      // Nothing to be done besides retrying later
      return;

    case ApiStatus::OK: // Cool beans
      return;

    case ApiStatus::MUST_UPGRADE:
      interruptEvent = InterruptEvent::MUST_UPGRADE;
      return;
    }

    this->logger.error(F("Unexpected status, EventLoop::handleMeasurements: "),
                       this->api.network().apiStatusToString(status));
  }
};

PROGMEM_STRING(missingHost, "No host available");
auto eventLoop = make_unique<EventLoop>(host.asRef().expect(missingHost));

void setup() noexcept { eventLoop->setup(); }
void loop() noexcept { eventLoop->loop(); }