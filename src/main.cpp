#include "configuration.hpp"
#include "flash.hpp"
#include "models.hpp"
#include "reset.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "api.hpp"

#include <optional>

#include "driver/device.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

class EventLoop {
private:
  Sensors sensors;
  Api api;
  CredentialsServer credentialsServer;
  iop::Log logger;

  Flash flash;

  iop::esp_time nextMeasurement;
  iop::esp_time nextYieldLog;
  iop::esp_time nextHandleConnectionLost;

public:
  void setup() noexcept {
    this->logger.info(F("Start Setup"));
    IOP_TRACE();
    pinMode(LED_BUILTIN, OUTPUT);

    Flash::setup();
    reset::setup();
    this->sensors.setup();
    this->api.setup();
    this->credentialsServer.setup();
    this->logger.info(F("Setup finished"));
  }

  void loop() noexcept {
    this->logger.trace(F("\n\n\n\n\n\n"));
    IOP_TRACE();
#ifdef LOG_MEMORY
    utils::logMemory(this->logger);
#endif

    const auto &authToken = this->flash.readAuthToken();

    // Handle all queued interrupts (only allows one of each kind concurrently)
    while (true) {
      auto ev = utils::descheduleInterrupt();
      if (ev == InterruptEvent::NONE)
        break;

      this->handleInterrupt(ev, authToken);
      yield();
    }

    const auto now = millis();

    const auto isConnected = iop::Network::isConnected();
    const auto hasAuthToken = authToken.has_value();
    if (isConnected && hasAuthToken)
      this->credentialsServer.close();

    if (!hasAuthToken) {
      this->handleCredentials();

    } else if (!isConnected) {
      // If connection is lost frequently we open the credentials server, to
      // allow replacing the wifi credentials. Since we only remove it
      // from flash if it's going to be replaced by a new one (allows
      // for more resiliency) - or during factory reset
      constexpr const uint32_t oneMinute = 60 * 1000;

      if (this->nextHandleConnectionLost == 0) {
        this->nextHandleConnectionLost = now + oneMinute;

      } else if (this->nextHandleConnectionLost < now) {
        this->logger.debug(F("Has creds, but no signal, opening server"));
        this->nextHandleConnectionLost = now + oneMinute;
        this->handleCredentials();

      } else {
        // No-op, we must just wait
      }

    } else if (this->nextMeasurement <= now) {
      this->nextHandleConnectionLost = 0;
      this->nextMeasurement = now + interval;
      this->handleMeasurements(iop::unwrap_ref(authToken, IOP_CTX()));
      //this->logger.info(std::to_string(ESP.getVcc())); // TODO: remove this
      
    } else if (this->nextYieldLog <= now) {
      this->nextHandleConnectionLost = 0;
      constexpr const uint16_t tenSeconds = 10000;
      this->nextYieldLog = now + tenSeconds;
      this->logger.trace(F("Waiting"));

    } else {
      this->nextHandleConnectionLost = 0;
    }
  }

private:
  void
  handleInterrupt(const InterruptEvent event,
                  const std::optional<AuthToken> &maybeToken) const noexcept {
    // Satisfies linter when all interrupt features are disabled
    (void)*this;
    (void)event;
    (void)maybeToken;

    IOP_TRACE();

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::FACTORY_RESET:
#ifdef IOP_FACTORY_RESET
      this->logger.warn(F("Factory Reset: deleting stored credentials"));
      this->flash.removeWifiConfig();
      this->flash.removeAuthToken();
      iop::Network::disconnect();
#endif
      (void)0; // Satisfies linter
      break;
    case InterruptEvent::MUST_UPGRADE:
#ifdef IOP_OTA
      if (maybeToken.has_value()) {
        const auto &token = iop::unwrap_ref(maybeToken, IOP_CTX());
        const auto status = this->api.upgrade(token);
        switch (status) {
        case iop::NetworkStatus::FORBIDDEN:
          this->logger.warn(F("Invalid auth token, but keeping since at OTA"));
          return;

        case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
          iop_panic(F("Api::upgrade internal buffer overflow"));

        // Already logged at the network level
        case iop::NetworkStatus::CONNECTION_ISSUES:
        case iop::NetworkStatus::BROKEN_SERVER:
          // Nothing to be done besides retrying later

        case iop::NetworkStatus::OK: // Cool beans
          return;
        }

        const auto str = iop::Network::apiStatusToString(status);
        this->logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
      } else {
        this->logger.error(
            F("Upgrade was expected, but no auth token was available"));
      }
#endif
      (void)1; // Satisfies linter
      break;
    case InterruptEvent::ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto ip = WiFi.localIP().toString();
      const auto status = std::to_string(wifi_station_get_connect_status());
      this->logger.debug(F("WiFi connected ("), ip, F("): "), status);

      struct station_config config = {0};
      wifi_station_get_config(&config);

      // We treat wifi credentials as a blob instead of worrying about encoding

      size_t len = sizeof(config.ssid);
      const auto *ptr = static_cast<uint8_t *>(config.ssid);
      const auto ssid = NetworkName::fromBytesUnsafe(ptr, len);

      const auto ssidStr = ssid.asString();
      this->logger.info(F("Connected to network: "), iop::to_view(ssidStr));

      len = sizeof(config.password);
      ptr = static_cast<uint8_t *>(config.password);
      const auto psk = NetworkPassword::fromBytesUnsafe(ptr, len);

      this->flash.writeWifiConfig(WifiCredentials(ssid, psk));
#endif
      (void)2; // Satisfies linter
      break;
    };
  }

  void handleCredentials() noexcept {
    IOP_TRACE();

    const auto &wifi = this->flash.readWifiConfig();
    const auto maybeToken = this->credentialsServer.serve(wifi, this->api);

    if (maybeToken.has_value())
      this->flash.writeAuthToken(iop::unwrap_ref(maybeToken, IOP_CTX()));
  }

  void handleMeasurements(const AuthToken &token) noexcept {
    IOP_TRACE();

    this->logger.debug(F("Handle Measurements"));

    const auto measurements = sensors.measure();
    const auto status = this->api.registerEvent(token, measurements);

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(F("Unable to send measurements"));
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash.removeAuthToken();
      return;

    case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
      this->logger.error(F("Unable to send measurements"));
      iop_panic(F("Api::registerEvent internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::BROKEN_SERVER:
    case iop::NetworkStatus::CONNECTION_ISSUES:
      // Nothing to be done besides retrying later

    case iop::NetworkStatus::OK: // Cool beans
      return;
    }

    this->logger.error(F("Unexpected status, EventLoop::handleMeasurements: "),
                       iop::Network::apiStatusToString(status));
  }

public:
  auto operator=(EventLoop const &other) noexcept -> EventLoop & {
    IOP_TRACE();
    if (this == &other)
      return *this;

    this->sensors = other.sensors;
    this->api = other.api;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash = other.flash;
    this->nextMeasurement = other.nextMeasurement;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  };
  auto operator=(EventLoop &&other) noexcept -> EventLoop & {
    IOP_TRACE();
    this->sensors = other.sensors;
    this->api = other.api;
    this->credentialsServer = other.credentialsServer;
    this->logger = other.logger;
    this->flash = other.flash;
    this->nextMeasurement = other.nextMeasurement;
    this->nextYieldLog = other.nextYieldLog;
    this->nextHandleConnectionLost = other.nextHandleConnectionLost;
    return *this;
  }
  ~EventLoop() noexcept { IOP_TRACE(); }
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : sensors(soilResistivityPowerPin, soilTemperaturePin,
                airTempAndHumidityPin, dhtVersion),
        api(std::move(uri), logLevel_), credentialsServer(logLevel_),
        logger(logLevel_, F("LOOP")), flash(logLevel_), nextMeasurement(0),
        nextYieldLog(0), nextHandleConnectionLost(0) {
    IOP_TRACE();
  }
  EventLoop(EventLoop const &other) noexcept
      : sensors(other.sensors), api(other.api),
        credentialsServer(other.credentialsServer), logger(other.logger),
        flash(other.flash), nextMeasurement(other.nextMeasurement),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }
  EventLoop(EventLoop &&other) noexcept
      : sensors(other.sensors), api(other.api),
        credentialsServer(other.credentialsServer), logger(other.logger),
        flash(other.flash), nextMeasurement(other.nextMeasurement),
        nextYieldLog(other.nextYieldLog),
        nextHandleConnectionLost(other.nextHandleConnectionLost) {
    IOP_TRACE();
  }
};

// Avoid static initialization being run before logging is setup
static std::optional<EventLoop> eventLoop;
void setup() {
  iop::Log::setup(logLevel);
  IOP_TRACE();
  eventLoop.emplace(uri(), logLevel);
  iop::unwrap_mut(eventLoop, IOP_CTX()).setup();
}

void loop() {
  iop::unwrap_mut(eventLoop, IOP_CTX()).loop();
}