// Those are here so we can hijack BearlSSL::CertStore and the F macro
#include "certificate_storage.hpp"

#include "Arduino.h"

#include "static_string.hpp"

// Now actual includes can start

#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"
#include "log.hpp"
#include "reset.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "static_string.hpp"
#include "utils.hpp"

#include "static_string.hpp"

class EventLoop {
private:
  Sensors sensors;
  Api api;
  CredentialsServer credentialsServer;
  Log logger;
  Flash flash;
  MD5Hash firmwareHash;

  esp_time nextTime;
  esp_time nextYieldLog;
  esp_time nextHandleConnectionLost;

public:
  void setup() noexcept {
    pinMode(LED_BUILTIN, OUTPUT);

    reset::setup();
    this->logger.setup();
    this->sensors.setup();
    Flash::setup();
    Api::setup();
    this->credentialsServer.setup();
  }

  void loop() noexcept {
#ifdef LOG_MEMORY
    this->logger.debug(F("Memory: "), String(ESP.getFreeHeap()), F(" "),
                       String(ESP.getFreeContStack()), F(" "),
                       ESP.getFreeSketchSpace());
#endif

    // TODO(pc): multiple sequenced interrupts may be lost
    this->handleInterrupt();

    const esp_time now = millis();
    const auto authToken = this->flash.readAuthToken();
    const auto plantId = this->flash.readPlantId();

    if (Api::isConnected() && authToken.isSome() && plantId.isSome()) {
      this->credentialsServer.close();
    }

    if (authToken.isNone()) {
      this->handleCredentials();

    } else if (plantId.isNone()) {
      this->handlePlant(UNWRAP_REF(authToken));

    } else if (!Api::isConnected()) {
      // If connection is lost frequently we open the credentials server, to
      // allow replacing the wifi credentials. Since we only remove it from
      // flash if we are going to replace it for a new one (allows for more
      // resiliency) - or during factory reset
      if (this->nextHandleConnectionLost < millis()) {
        constexpr const uint32_t oneMinute = 60 * 1000;
        this->nextHandleConnectionLost = millis() + oneMinute;
        this->handleCredentials();
      } else {
        // No-op, we must just wait
      }

    } else if (this->nextTime <= now) {
      this->nextTime = millis() + interval;
      this->handleMeasurements(UNWRAP_REF(authToken), UNWRAP_REF(plantId));

    } else if (this->nextYieldLog <= now) {
      constexpr const uint16_t oneSecond = 1000;
      this->nextYieldLog = now + oneSecond;
      this->logger.trace(F("Waiting"));
    }
  }

  auto operator=(EventLoop const &other) -> EventLoop & = delete;
  auto operator=(EventLoop &&other) -> EventLoop & = delete;
  ~EventLoop() = default;
  explicit EventLoop(const StaticString host) noexcept
      : sensors(soilResistivityPowerPin, soilTemperaturePin,
                airTempAndHumidityPin, dhtVersion),
        api(host, logLevel), credentialsServer(logLevel),
        logger(logLevel, F("LOOP")), flash(logLevel),
        firmwareHash(hashSketch()), nextTime(0), nextYieldLog(0),
        nextHandleConnectionLost(0) {}
  EventLoop(EventLoop const &other) = delete;
  EventLoop(EventLoop &&other) = delete;

private:
  void handleInterrupt() const noexcept {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
    case NONE:
      break;
    case FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.warn(F("Factory Reset: deleting stored credentials"));
      this->flash.removeWifiConfig();
      this->flash.removeAuthToken();
      this->flash.removePlantId();
      Api::disconnect();
#endif
      break;
    case MUST_UPGRADE:
#ifdef IOP_OTA
      const auto maybeToken = this->flash.readAuthToken();
      const auto id = this->flash.readPlantId();
      if (maybeToken.isSome()) {
        const auto &token = UNWRAP_REF(maybeToken);
        const auto status = this->api.upgrade(token, id, this->firmwareHash);
        switch (status) {
        case ApiStatus::FORBIDDEN:
          this->logger.warn(F("Invalid auth token, but keeping since at OTA"));
          return;

        case ApiStatus::NOT_FOUND:
          // No update for this plant
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
        this->logger.warn(
            F("Upgrade was expected, but no auth token was available"));
      }
#endif
      break;
    case ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto ip = WiFi.localIP().toString();
      const auto status = std::to_string(wifi_station_get_connect_status());
      this->logger.debug(F("WiFi connected ("), ip, F("): "), status);

      struct station_config config = {0};
      wifi_station_get_config(&config);

      const auto rawSsid =
          UnsafeRawString(reinterpret_cast<char *>(config.ssid));
      const auto ssid = NetworkName::fromStringTruncating(rawSsid);
      this->logger.info(F("Connected to network:"), ssid.asString());

      const auto rawPsk =
          UnsafeRawString(reinterpret_cast<char *>(config.password));
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

  void handleCredentials() noexcept {
    const auto wifi = this->flash.readWifiConfig();
    const auto maybeToken = this->credentialsServer.serve(wifi, this->api);

    if (maybeToken.isSome())
      this->flash.writeAuthToken(UNWRAP_REF(maybeToken));
  }

  void handlePlant(const AuthToken &token) const noexcept {
    const auto maybePlantId = this->api.registerPlant(token);

    if (IS_ERR(maybePlantId)) {
      this->logger.error(F("Unable to get plant id"));

      const auto &status = UNWRAP_ERR_REF(maybePlantId);

      const uint32_t oneMinutesUs = 60 * 1000 * 1000;
      const uint32_t fiveSeconds = 5 * 1000;

      switch (status) {
      case ApiStatus::FORBIDDEN:
        this->logger.warn(F("Auth token was refused, deleting it"));
        this->flash.removeAuthToken();
        return;

      case ApiStatus::CLIENT_BUFFER_OVERFLOW:
        panic_(F("Api::registerPlant internal buffer overflow"));

      case ApiStatus::NOT_FOUND:
        this->logger.error(F("Api::registerPlant: 404, server is broken"));

      case ApiStatus::BROKEN_SERVER:
        // Server is broken. Nothing we can do besides waiting
        // Since this method is only called when nothing can be done without
        // it, we sleep to avoid constant retries
        ESP.deepSleep(oneMinutesUs);
        return;

      case ApiStatus::BROKEN_PIPE:
      case ApiStatus::TIMEOUT:
      case ApiStatus::NO_CONNECTION:
        // Nothing to be done besides retrying later
        // Since this method is only called when nothing can be done without
        // it, we sleep to avoid constant retries
        delay(fiveSeconds);
        return;

      case ApiStatus::OK:
      case ApiStatus::MUST_UPGRADE:
        // On success ApiStatus won't be returned
        panic_(F("Unreachable"));
        return;
      }

      const auto s = Network::apiStatusToString(status);
      this->logger.error(F("Unexpected status, Api::registerPlants: "), s);

    } else {
      this->flash.writePlantId(UNWRAP_OK_REF(maybePlantId));
    }
  }

  void handleMeasurements(const AuthToken &token, const PlantId &id) noexcept {
    this->logger.debug(F("Handle Measurements"));

    const auto measurements = sensors.measure(id, this->firmwareHash);
    const auto status = this->api.registerEvent(token, measurements);

    switch (status) {
    case ApiStatus::FORBIDDEN:
      // TODO: is there a way to keep it around somewhere as a backup?
      // In case it's a server bug, since it will force the user to join our AP
      // again to retype the credentials. If so how can we reuse it and detect
      // it started working again?
      this->logger.error(F("Unable to send measurements"));
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
      return;

    case ApiStatus::NOT_FOUND:
      //  There is no problem removing it since we can just request for another
      //  with our MAC ADDRESS, but may indeed cause problems with things like
      //  error logging and panicking if it's removed (TODO), should we just
      //  depend on mac address (and auth token)? Or maybe device id?
      this->logger.error(F("Unable to send measurements"));
      this->logger.warn(F("Plant Id was not found, deleting it"));
      this->flash.removePlantId();
      return;

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      this->logger.error(F("Unable to send measurements"));
      panic_(F("Api::registerEvent internal buffer overflow"));

    case ApiStatus::BROKEN_SERVER: // This already is logged at Network
    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      this->logger.error(F("Unable to send measurements"));

    case ApiStatus::OK: // Cool beans
      return;

    case ApiStatus::MUST_UPGRADE:
      interruptEvent = InterruptEvent::MUST_UPGRADE;
      return;
    }

    this->logger.error(F("Unexpected status, EventLoop::handleMeasurements: "),
                       Network::apiStatusToString(status));
  }
};

PROGMEM_STRING(missingHost, "No host available");
static EventLoop eventLoop(host.asRef().expect(missingHost));

void setup() { eventLoop.setup(); }
void loop() { eventLoop.loop(); }