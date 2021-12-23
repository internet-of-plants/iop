#include "loop.hpp" 
#include "core/panic.hpp"

EventLoop eventLoop(config::uri(), config::logLevel);

void EventLoop::setup() noexcept {
  IOP_TRACE();

  this->logger.info(FLASH("Start Setup"));
  gpio::gpio.mode(gpio::LED_BUILTIN, gpio::Mode::OUTPUT);

  Flash::setup();
  reset::setup();
  this->sensors.setup();
  this->api().setup();
  this->credentialsServer.setup();
  (void)iop::data; // sets Raw Data so drivers can access the HttpClient, it's a horrible hack;
  this->logger.info(FLASH("Setup finished"));
  this->logger.info(FLASH("MD5: "), iop::to_view(driver::device.binaryMD5()));
}

constexpr static uint64_t intervalTryFlashWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedIopCredentialsMillis =
    60 * 60 * 1000; // 1 hour

void EventLoop::loop() noexcept {
    this->logger.trace(FLASH("\n\n\n\n\n\n"));
    IOP_TRACE();
#ifdef LOG_MEMORY
    iop::logMemory(this->logger);
#endif

    const auto authToken = this->flash().readAuthToken();

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

    if (isConnected && authToken)
        this->credentialsServer.close();

    if (isConnected && this->nextNTPSync < now) {
      constexpr const uint32_t sixHours = 6 * 60 * 60 * 1000;
      this->logger.info(FLASH("Syncing NTP"));
      // UTC by default, should we change according to the user? We currently only use this to validate SSL cert dates
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      this->nextNTPSync = now + sixHours;
      this->logger.info(FLASH("Time synced"));

    } else if (isConnected && !authToken) {
        this->handleIopCredentials();

    } else if (!isConnected) {
        this->handleNotConnected();

    } else if (this->nextMeasurement <= now) {
      this->nextHandleConnectionLost = 0;
      this->nextMeasurement = now + config::interval;
      iop_assert(authToken, FLASH("Auth Token not found"));
      this->handleMeasurements(*authToken);
      //this->logger.info(std::to_string(ESP.getVcc())); // TODO: remove this
        
    } else if (this->nextYieldLog <= now) {
      this->nextHandleConnectionLost = 0;
      constexpr const uint16_t tenSeconds = 10000;
      this->nextYieldLog = now + tenSeconds;
      this->logger.trace(FLASH("Waiting"));

    } else {
      this->nextHandleConnectionLost = 0;
    }
}

void EventLoop::handleNotConnected() noexcept {
  IOP_TRACE();

  const auto now = driver::thisThread.now();

  // If connection is lost frequently we open the credentials server, to
  // allow replacing the wifi credentials. Since we only remove it
  // from flash if it's going to be replaced by a new one (allows
  // for more resiliency) - or during factory reset
  constexpr const uint32_t oneMinute = 60 * 1000;

  const auto &wifi = this->flash().readWifiConfig();

  const auto isConnected = iop::Network::isConnected();
  if (!isConnected && wifi && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = wifi->get();
    const auto ssid = std::string_view(stored.ssid.get().data(), stored.ssid.get().max_size());
    const auto psk = std::string_view(stored.password.get().data(), stored.password.get().max_size());
    this->logger.info(FLASH("Trying wifi credentials stored in flash: "), iop::to_view(iop::scapeNonPrintable(ssid)));
    this->logger.debug(FLASH("Password:"), iop::to_view(iop::scapeNonPrintable(psk)));
    this->connect(ssid, psk);

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but that's the price of hardcoding, if it's
    // not updated it may just be. Since it can't be deleted we must retry,
    // but with a bigish interval.



  } else if (!isConnected && config::wifiNetworkName() && config::wifiPassword() && this->nextTryHardcodedWifiCredentials <= now) { 
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;

    this->logger.info(FLASH("Trying hardcoded wifi credentials"));

    const auto ssid = *config::wifiNetworkName();
    const auto psk = *config::wifiPassword();
    this->connect(ssid.toString(), psk.toString());




  } else if (this->nextHandleConnectionLost < now) {
    this->logger.debug(FLASH("Has creds, but no signal, opening server"));
    this->nextHandleConnectionLost = now + oneMinute;
    this->handleCredentials();

  } else {
    // No-op, we must just wait
  }
}

void EventLoop::handleIopCredentials() noexcept {
  IOP_TRACE();

  const auto now = driver::thisThread.now();

  if (config::iopEmail() && config::iopPassword() && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;

    this->logger.info(FLASH("Trying hardcoded iop credentials"));

    const auto email = *config::iopEmail();
    const auto password = *config::iopPassword();
    const auto tok = this->authenticate(email.toString(), password.toString(), this->api());
    if (tok)
      this->flash().writeAuthToken(*tok);
  } else {
    this->handleCredentials();
  }
}

void EventLoop::handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &maybeToken) const noexcept {
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
      this->logger.warn(FLASH("Factory Reset: deleting stored credentials"));
      this->flash().removeWifiConfig();
      this->flash().removeAuthToken();
      iop::Network::disconnect();
#endif
      (void)0; // Satisfies linter
      break;
    case InterruptEvent::MUST_UPGRADE:
#ifdef IOP_OTA
      if (maybeToken) {
        const auto &token = *maybeToken;
        const auto status = this->api().upgrade(token);
        switch (status) {
        case iop::NetworkStatus::FORBIDDEN:
          this->logger.warn(FLASH("Invalid auth token, but keeping since at OTA"));
          return;

        case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
          iop_panic(FLASH("Api::upgrade internal buffer overflow"));

        // Already logged at the network level
        case iop::NetworkStatus::CONNECTION_ISSUES:
        case iop::NetworkStatus::BROKEN_SERVER:
          // Nothing to be done besides retrying later

        case iop::NetworkStatus::OK: // Cool beans
          return;
        }

        const auto str = iop::Network::apiStatusToString(status);
        this->logger.error(FLASH("Bad status, EventLoop::handleInterrupt "), str);
      } else {
        this->logger.error(
            FLASH("Upgrade was expected, but no auth token was available"));
      }
#endif
      (void)1; // Satisfies linter
      break;
    case InterruptEvent::ON_CONNECTION:
#ifdef IOP_ONLINE
      const auto ip = iop::data.wifi.localIP();
      const auto status = this->statusToString(iop::data.wifi.status());
      this->logger.debug(FLASH("WiFi connected ("), iop::to_view(ip), FLASH("): "), status.value_or(iop::StaticString(FLASH("BadData"))));

      const auto config = iop::data.wifi.credentials();
      // We treat wifi credentials as a blob instead of worrying about encoding

      unused4KbSysStack.ssid().fill('\0');
      memcpy(unused4KbSysStack.ssid().data(), config.first.begin(), sizeof(config.first));
      this->logger.info(FLASH("Connected to network: "), iop::to_view(iop::scapeNonPrintable(std::string_view(unused4KbSysStack.ssid().data(), 32))));

      unused4KbSysStack.psk().fill('\0');
      memcpy(unused4KbSysStack.psk().data(), config.second.begin(), sizeof(config.second));
      this->flash().writeWifiConfig(WifiCredentials(unused4KbSysStack.ssid(), unused4KbSysStack.psk()));
#endif
      (void)2; // Satisfies linter
      break;
    };
}

void EventLoop::handleCredentials() noexcept {
    IOP_TRACE();

    const auto token = this->credentialsServer.serve(this->api());

    if (token)
      this->flash().writeAuthToken(*token);
}

void EventLoop::handleMeasurements(const AuthToken &token) noexcept {
    IOP_TRACE();

    this->logger.debug(FLASH("Handle Measurements"));

    const auto measurements = sensors.measure();
    const auto status = this->api().registerEvent(token, measurements);

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(FLASH("Unable to send measurements"));
      this->logger.warn(FLASH("Auth token was refused, deleting it"));
      this->flash().removeAuthToken();
      return;

    case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
      this->logger.error(FLASH("Unable to send measurements"));
      iop_panic(FLASH("Api::registerEvent internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::BROKEN_SERVER:
    case iop::NetworkStatus::CONNECTION_ISSUES:
      // Nothing to be done besides retrying later

    case iop::NetworkStatus::OK: // Cool beans
      return;
    }

    this->logger.error(FLASH("Unexpected status, EventLoop::handleMeasurements: "),
                       iop::Network::apiStatusToString(status));
}


void EventLoop::connect(std::string_view ssid,
                                std::string_view password) const noexcept {
  IOP_TRACE();
  this->logger.info(FLASH("Connect: "), ssid);
  if (iop::data.wifi.status() == driver::StationStatus::CONNECTING) {
    iop::data.wifi.stationDisconnect();
  }

  if (!iop::data.wifi.begin(ssid, password)) {
    this->logger.error(FLASH("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::data.wifi.status();
    auto statusStr = this->statusToString(status);
    if (!statusStr)
      return; // It already will be logged by statusToString;

    this->logger.error(FLASH("Invalid wifi credentials ("), *statusStr, FLASH("): "), std::move(ssid));
  }
}

auto EventLoop::authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::optional<AuthToken> {
  IOP_TRACE();

  iop::data.wifi.setMode(driver::WiFiMode::STA);
  auto authToken = api.authenticate(username, std::move(password));
  iop::data.wifi.setMode(driver::WiFiMode::AP_STA);

  this->logger.info(FLASH("Tried to authenticate"));
  if (const auto *error = std::get_if<iop::NetworkStatus>(&authToken)) {
    const auto &status = *error;

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(FLASH("Invalid IoP credentials ("), iop::Network::apiStatusToString(status), FLASH("): "), username);
      return std::nullopt;

    case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
      iop_panic(FLASH("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::CONNECTION_ISSUES:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return std::nullopt;

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(FLASH("Unreachable"));
    }

    const auto str = iop::Network::apiStatusToString(status);
    this->logger.crit(FLASH("CredentialsServer::authenticate bad status: "), str);
    return std::nullopt;

  } else if (auto *token = std::get_if<AuthToken>(&authToken)) {
    return *token;
  }

  iop_panic(FLASH("Invalid variant"));
}

auto EventLoop::statusToString(const driver::StationStatus status) const noexcept -> std::optional<iop::StaticString> {
  IOP_TRACE();

  switch (status) {
  case driver::StationStatus::IDLE:
    return FLASH("STATION_IDLE");

  case driver::StationStatus::CONNECTING:
    return FLASH("STATION_CONNECTING");

  case driver::StationStatus::WRONG_PASSWORD:
    return FLASH("STATION_WRONG_PASSWORD");

  case driver::StationStatus::NO_AP_FOUND:
    return FLASH("STATION_NO_AP_FOUND");

  case driver::StationStatus::CONNECT_FAIL:
    return FLASH("STATION_CONNECT_FAIL");

  case driver::StationStatus::  GOT_IP:
    return FLASH("STATION_GOT_IP");
  }

  this->logger.error(iop::StaticString(FLASH("Unknown status: ")).toString() + std::to_string(static_cast<uint8_t>(status)));
  return std::nullopt;
}