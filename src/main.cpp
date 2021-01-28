// Those are here so we can hijack BearlSSL::CertStore
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

#include "string_view.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

class EventLoop {
private:
  Sensors sensors;
  Api api;
  CredentialsServer credentialsServer;
  Log logger;

  Flash flash;
  MD5Hash firmwareHash = MD5Hash::empty();
  MacAddress macAddress;

  esp_time nextTime;
  esp_time nextYieldLog;
  esp_time nextHandleConnectionLost;

public:
  void setup() noexcept {
    IOP_TRACE();
    pinMode(LED_BUILTIN, OUTPUT);

    Flash::setup();
    reset::setup();
    this->sensors.setup();
    this->api.setup();
    this->credentialsServer.setup();
  }

  void loop() noexcept {
    IOP_TRACE();
#ifdef LOG_MEMORY
    utils::logMemory(this->logger);
#endif
    const auto authToken = this->flash.readAuthToken();

    // Handle all queued interrupts (only allows one of each kind concurrently)
    while (true) {
      auto ev = utils::descheduleInterrupt();
      if (ev == InterruptEvent::NONE)
        break;

      this->handleInterrupt(ev, authToken);
      yield();
    }

    const auto now = millis();

    if (Network::isConnected() && authToken.isSome())
      this->credentialsServer.close();

    if (authToken.isNone()) {
      this->handleCredentials();

    } else if (!Network::isConnected()) {
      // If connection is lost frequently: we open the credentials server, to
      // allow replacing the wifi credentials. Since we only remove it
      // from flash if it's going to be replaced for a new one (allows
      // for more resiliency) - or during factory reset
      constexpr const uint32_t oneMinute = 60 * 1000;

      if (this->nextHandleConnectionLost == 0) {
        this->nextHandleConnectionLost = now + oneMinute;

      } else if (this->nextHandleConnectionLost < now) {
        this->logger.debug(
            F("Network credentials available, but no wifi, opening server"));
        this->nextHandleConnectionLost = now + oneMinute;
        this->handleCredentials();

      } else {
        // No-op, we must just wait
      }

    } else if (this->nextTime <= now) {
      this->nextHandleConnectionLost = 0;
      this->nextTime = now + interval;
      this->handleMeasurements(UNWRAP_REF(authToken));

    } else if (this->nextYieldLog <= now) {
      this->nextHandleConnectionLost = 0;
      constexpr const uint16_t tenSeconds = 10000;
      this->nextYieldLog = now + tenSeconds;
      this->logger.trace(F("Waiting"));

    } else {
      this->nextHandleConnectionLost = 0;
    }
  }

  auto operator=(EventLoop const &other) -> EventLoop & {
    IOP_TRACE();
    this->sensors = other.sensors;
    this->api = other.api;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash = other.flash;
    this->firmwareHash = other.firmwareHash;
    this->macAddress = other.macAddress;
    this->nextTime = other.nextTime;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  };
  auto operator=(EventLoop &&other) -> EventLoop & {
    IOP_TRACE();
    this->sensors = other.sensors;
    this->api = other.api;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash = other.flash;
    this->firmwareHash = other.firmwareHash;
    this->macAddress = other.macAddress;
    this->nextTime = other.nextTime;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  }
  ~EventLoop() { IOP_TRACE(); };
  explicit EventLoop(const StaticString host) noexcept
      : sensors(soilResistivityPowerPin, soilTemperaturePin,
                airTempAndHumidityPin, dhtVersion),
        api(host, logLevel), credentialsServer(logLevel),
        logger(logLevel, F("LOOP")), flash(logLevel),
        firmwareHash(utils::hashSketch()), macAddress(utils::macAddress()),
        nextTime(0), nextYieldLog(0), nextHandleConnectionLost(0) {
    IOP_TRACE();
  }
  EventLoop(EventLoop const &other) noexcept
      : sensors(other.sensors), api(other.api),
        credentialsServer(other.credentialsServer), logger(other.logger),
        flash(other.flash), firmwareHash(other.firmwareHash),
        macAddress(other.macAddress), nextTime(other.nextTime),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }
  EventLoop(EventLoop &&other) noexcept
      : sensors(other.sensors), api(other.api),
        credentialsServer(other.credentialsServer), logger(other.logger),
        flash(other.flash), firmwareHash(other.firmwareHash),
        macAddress(other.macAddress), nextTime(other.nextTime),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }

private:
  void handleInterrupt(const InterruptEvent event,
                       const Option<AuthToken> &maybeToken) const noexcept {
    IOP_TRACE();

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.warn(F("Factory Reset: deleting stored credentials"));
      this->flash.removeWifiConfig();
      this->flash.removeAuthToken();
      Network::disconnect();
#endif
      break;
    case InterruptEvent::MUST_UPGRADE:
#ifdef IOP_OTA
      if (maybeToken.isSome()) {
        const auto &token = UNWRAP_REF(maybeToken);
        const auto mac = this->macAddress;
        const auto status = this->api.upgrade(token, mac, this->firmwareHash);
        switch (status) {
        case ApiStatus::FORBIDDEN:
          this->logger.warn(F("Invalid auth token, but keeping since at OTA"));
          return;

        case ApiStatus::NOT_FOUND:
          // No update for this plant

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

        case ApiStatus::OK: // Cool beans
          return;
        }

        const auto str = Network::apiStatusToString(status);
        this->logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
      } else {
        this->logger.error(
            F("Upgrade was expected, but no auth token was available"));
      }
#endif
      break;
    case InterruptEvent::ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto ip = WiFi.localIP().toString();
      const auto status = std::to_string(wifi_station_get_connect_status());
      this->logger.debug(F("WiFi connected ("), ip, F("): "), status);

      struct station_config config = {0};
      wifi_station_get_config(&config);

      const auto rawSsid =
          UnsafeRawString(reinterpret_cast<char *>(config.ssid));
      const auto ssid = NetworkName::fromStringTruncating(rawSsid);
      this->logger.info(F("Connected to network: "), ssid.asString());

      const auto rawPsk =
          UnsafeRawString(reinterpret_cast<char *>(config.password));
      const auto psk = NetworkPassword::fromStringTruncating(rawPsk);

      this->flash.writeWifiConfig({.ssid = ssid, .password = psk});
#endif
      break;
    };
  }

  void handleCredentials() noexcept {
    IOP_TRACE();

    const auto wifi = this->flash.readWifiConfig();
    const auto maybeToken =
        this->credentialsServer.serve(wifi, this->macAddress, this->api);

    if (maybeToken.isSome())
      this->flash.writeAuthToken(UNWRAP_REF(maybeToken));
  }

  void handleMeasurements(const AuthToken &token) noexcept {
    IOP_TRACE();

    this->logger.debug(F("Handle Measurements"));

    const auto measurements =
        sensors.measure(this->macAddress, this->firmwareHash);
    const auto status = this->api.registerEvent(token, measurements);

    switch (status) {
    case ApiStatus::FORBIDDEN:
      // TODO: is there a way to keep it around somewhere as a backup?
      // In case it's a server bug, since it will force the user to join our AP
      // again to retype the credentials. If so how can we reuse it and
      // detect it started working again?
      this->logger.error(F("Unable to send measurements"));
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
      return;

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      this->logger.error(F("Unable to send measurements"));
      panic_(F("Api::registerEvent internal buffer overflow"));

    case ApiStatus::BROKEN_SERVER: // This already is logged at Network
    case ApiStatus::NOT_FOUND:     // Should never happen
    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      this->logger.error(F("Unable to send measurements: "),
                         Network::apiStatusToString(status));

    case ApiStatus::OK: // Cool beans
      return;

    case ApiStatus::MUST_UPGRADE:
      utils::scheduleInterrupt(InterruptEvent::MUST_UPGRADE);
      return;
    }

    this->logger.error(F("Unexpected status, EventLoop::handleMeasurements: "),
                       Network::apiStatusToString(status));
  }
};

// Avoid static initialization being run before logging is setup
static Option<EventLoop> eventLoop;
void setup() {
  Log::setup();
  IOP_TRACE();
  eventLoop = EventLoop(UNWRAP_REF(host));
  UNWRAP_MUT(eventLoop).setup();
}

void loop() {
  IOP_TRACE();
  UNWRAP_MUT(eventLoop).loop();
}