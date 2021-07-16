#include "loop.hpp" 

void EventLoop::setup() noexcept {
    IOP_TRACE();

    this->logger.info(F("Start Setup"));
    gpio::gpio.mode(gpio::LED_BUILTIN, gpio::Mode::OUTPUT);

    Flash::setup();
    reset::setup();
    this->sensors.setup();
    this->api().setup();
    this->credentialsServer.setup();
    this->logger.info(F("Setup finished"));
}

void EventLoop::loop() noexcept {
    this->logger.trace(F("\n\n\n\n\n\n"));
    IOP_TRACE();
#ifdef LOG_MEMORY
    iop::logMemory(this->logger);
#endif

    const auto &authToken = this->flash().readAuthToken();

    // Handle all queued interrupts (only allows one of each kind concurrently)
    while (true) {
        auto ev = utils::descheduleInterrupt();
        if (ev == InterruptEvent::NONE)
          break;

        this->handleInterrupt(ev, authToken);
        driver::thisThread.yield();
    }

    const auto now = driver::thisThread.now();

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
        this->nextMeasurement = now + config::interval;
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

void EventLoop::handleInterrupt(const InterruptEvent event,
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
      this->flash().removeWifiConfig();
      this->flash().removeAuthToken();
      iop::Network::disconnect();
#endif
      (void)0; // Satisfies linter
      break;
    case InterruptEvent::MUST_UPGRADE:
#ifdef IOP_OTA
      if (maybeToken.has_value()) {
        const auto &token = iop::unwrap_ref(maybeToken, IOP_CTX());
        const auto status = this->api().upgrade(token);
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
      const auto status = this->credentialsServer.statusToString(driver::wifi.status());
      this->logger.debug(F("WiFi connected ("), iop::to_view(ip), F("): "), status.value_or(iop::StaticString(F("BadData"))));

      const auto config = driver::wifi.credentials();
      // We treat wifi credentials as a blob instead of worrying about encoding

      unused4KbSysStack.ssid().fill('\0');
      iop_assert(config.first.length() == 32, F("\0 inside SSID is not supported")); // this forbids \0 in ssids, pls no
      memcpy(unused4KbSysStack.ssid().data(), config.first.c_str(), config.first.length());
      this->logger.info(F("Connected to network: "), iop::to_view(iop::scapeNonPrintable(std::string_view(unused4KbSysStack.ssid().data(), 32))));

      unused4KbSysStack.psk().fill('\0');
      iop_assert(config.second.length() == 64, F("\0 inside PSK is not supported")); // this forbids \0 in ssids, pls no
      memcpy(unused4KbSysStack.psk().data(), config.second.c_str(), config.second.length());
      this->flash().writeWifiConfig(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));
#endif
      (void)2; // Satisfies linter
      break;
    };
}

void EventLoop::handleCredentials() noexcept {
    IOP_TRACE();

    const auto &wifi = this->flash().readWifiConfig();
    const auto maybeToken = this->credentialsServer.serve(wifi, this->api());

    if (maybeToken.has_value())
      this->flash().writeAuthToken(iop::unwrap_ref(maybeToken, IOP_CTX()));
}

void EventLoop::handleMeasurements(const AuthToken &token) noexcept {
    IOP_TRACE();

    this->logger.debug(F("Handle Measurements"));

    const auto measurements = sensors.measure();
    const auto status = this->api().registerEvent(token, measurements);

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(F("Unable to send measurements"));
      this->logger.warn(F("Auth token was refused, deleting it"));
      this->flash().removeAuthToken();
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