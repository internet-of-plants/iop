#include "loop.hpp" 

EventLoop eventLoop(config::uri(), config::logLevel);

void EventLoop::setup() noexcept {
  IOP_TRACE();

  this->logger.info(F("Start Setup"));
  gpio::gpio.mode(gpio::LED_BUILTIN, gpio::Mode::OUTPUT);

  Flash::setup();
  reset::setup();
  this->sensors.setup();
  this->api().setup();
  this->credentialsServer.setup();
  (void)iop::data; // sets Raw Data so drivers can access the HttpClient, it's a horrible hack;
  this->logger.info(F("Setup finished"));
  this->logger.info(F("MD5: "), iop::to_view(driver::device.binaryMD5()));
}

constexpr static uint64_t intervalTryFlashWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedIopCredentialsMillis =
    60 * 60 * 1000; // 1 hour

void EventLoop::loop() noexcept {
    this->logger.trace(F("\n\n\n\n\n\n"));
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
    const auto hasAuthToken = authToken.has_value();

    if (isConnected && hasAuthToken)
        this->credentialsServer.close();

    if (isConnected && this->nextNTPSync < now) {
      constexpr const uint32_t sixHours = 6 * 60 * 60 * 1000;
      this->logger.info(F("Syncing NTP"));
      // UTC by default, should we change according to the user? We currently only use this to validate SSL cert dates
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      this->nextNTPSync = now + sixHours;
      this->logger.info(F("Time synced"));

    } else if (isConnected && !hasAuthToken) {
        this->handleIopCredentials();

    } else if (!isConnected) {
        this->handleNotConnected();

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

void EventLoop::handleNotConnected() noexcept {
  IOP_TRACE();

  const auto now = driver::thisThread.now();

  // If connection is lost frequently we open the credentials server, to
  // allow replacing the wifi credentials. Since we only remove it
  // from flash if it's going to be replaced by a new one (allows
  // for more resiliency) - or during factory reset
  constexpr const uint32_t oneMinute = 60 * 1000;

  const auto &wifi = this->flash().readWifiConfig();
  const auto hasHardcodedWifiCreds = config::wifiNetworkName().has_value() && config::wifiPassword().has_value();

  const auto isConnected = iop::Network::isConnected();
  if (!isConnected && wifi.has_value() && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = iop::unwrap_ref(wifi, IOP_CTX()).get();
    const auto ssid = std::string_view(stored.ssid.get().data(), stored.ssid.get().max_size());
    const auto psk = std::string_view(stored.password.get().data(), stored.password.get().max_size());
    this->logger.info(F("Trying wifi credentials stored in flash: "), iop::to_view(iop::scapeNonPrintable(ssid)));
    this->logger.debug(F("Password:"), iop::to_view(iop::scapeNonPrintable(psk)));
    this->connect(ssid, psk);

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but that's the price of hardcoding, if it's
    // not updated it may just be. Since it can't be deleted we must retry,
    // but with a bigish interval.



  } else if (!isConnected && hasHardcodedWifiCreds && this->nextTryHardcodedWifiCredentials <= now) { 
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;

    this->logger.info(F("Trying hardcoded wifi credentials"));

    const auto ssid = iop::unwrap_ref(config::wifiNetworkName(), IOP_CTX());
    const auto psk = iop::unwrap_ref(config::wifiPassword(), IOP_CTX());
    this->connect(ssid.toString(), psk.toString());




  } else if (this->nextHandleConnectionLost < now) {
    this->logger.debug(F("Has creds, but no signal, opening server"));
    this->nextHandleConnectionLost = now + oneMinute;
    this->handleCredentials();

  } else {
    // No-op, we must just wait
  }
}

void EventLoop::handleIopCredentials() noexcept {
  IOP_TRACE();

  const auto now = driver::thisThread.now();

  const auto hasHardcodedIopCreds = config::iopEmail().has_value() && config::iopPassword().has_value();
  if (hasHardcodedIopCreds && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;

    this->logger.info(F("Trying hardcoded iop credentials"));

    const auto email = iop::unwrap_ref(config::iopEmail(), IOP_CTX());
    const auto password = iop::unwrap_ref(config::iopPassword(), IOP_CTX());
    const auto tok = this->authenticate(email.toString(), password.toString(), this->api());
    if (tok.has_value())
      this->flash().writeAuthToken(iop::unwrap_ref(tok, IOP_CTX()));
  } else {
    this->handleCredentials();
  }
}

void EventLoop::handleInterrupt(const InterruptEvent event,
                  const std::optional<std::reference_wrapper<const AuthToken>> &maybeToken) const noexcept {
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
      const auto ip = iop::data.wifi.localIP();
      const auto status = this->statusToString(iop::data.wifi.status());
      this->logger.debug(F("WiFi connected ("), iop::to_view(ip), F("): "), status.value_or(iop::StaticString(F("BadData"))));

      const auto config = iop::data.wifi.credentials();
      // We treat wifi credentials as a blob instead of worrying about encoding

      unused4KbSysStack.ssid().fill('\0');
      memcpy(unused4KbSysStack.ssid().data(), config.first.begin(), sizeof(config.first));
      this->logger.info(F("Connected to network: "), iop::to_view(iop::scapeNonPrintable(std::string_view(unused4KbSysStack.ssid().data(), 32))));

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

    const auto maybeToken = this->credentialsServer.serve(this->api());

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


void EventLoop::connect(std::string_view ssid,
                                std::string_view password) const noexcept {
  IOP_TRACE();
  this->logger.info(F("Connect: "), ssid);
  if (iop::data.wifi.status() == driver::StationStatus::CONNECTING) {
    iop::data.wifi.stationDisconnect();
  }

  if (!iop::data.wifi.begin(ssid, password)) {
    this->logger.error(F("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::data.wifi.status();
    auto maybeStatusStr = this->statusToString(status);
    if (!maybeStatusStr.has_value())
      return; // It already will be logged by statusToString;

    const auto statusStr = iop::unwrap(maybeStatusStr, IOP_CTX());
    this->logger.error(F("Invalid wifi credentials ("), statusStr, F("): "), std::move(ssid));
  }
}

auto EventLoop::authenticate(std::string_view username,
                                     std::string_view password,
                                     const Api &api) const noexcept
    -> std::optional<std::array<char, 64>> {
  IOP_TRACE();
  iop::data.wifi.setMode(driver::WiFiMode::STA);

  auto authToken = api.authenticate(username, std::move(password));
  
  iop::data.wifi.setMode(driver::WiFiMode::AP_STA);
  this->logger.info(F("Tried to authenticate"));
  if (iop::is_err(authToken)) {
    const auto &status = iop::unwrap_err_ref(authToken, IOP_CTX());

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger.error(F("Invalid IoP credentials ("),
                         iop::Network::apiStatusToString(status), F("): "),
                         std::move(username));
      return std::optional<AuthToken>();

    case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
      iop_panic(F("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::CONNECTION_ISSUES:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return std::optional<AuthToken>();

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(F("Unreachable"));
    }

    const auto str = iop::Network::apiStatusToString(status);
    this->logger.crit(F("CredentialsServer::authenticate bad status: "), str);
    return std::optional<AuthToken>();
  }
  return std::make_optional(std::move(iop::unwrap_ok_mut(authToken, IOP_CTX())));
}

auto EventLoop::statusToString(const driver::StationStatus status)
    const noexcept -> std::optional<iop::StaticString> {
  std::optional<iop::StaticString> ret;
  IOP_TRACE();
  switch (status) {
  case driver::StationStatus::IDLE:
    ret.emplace(F("STATION_IDLE"));
    break;
  case driver::StationStatus::CONNECTING:
    ret.emplace(F("STATION_CONNECTING"));
    break;
  case driver::StationStatus::WRONG_PASSWORD:
    ret.emplace(F("STATION_WRONG_PASSWORD"));
    break;
  case driver::StationStatus::NO_AP_FOUND:
    ret.emplace(F("STATION_NO_AP_FOUND"));
    break;
  case driver::StationStatus::CONNECT_FAIL:
    ret.emplace(F("STATION_CONNECT_FAIL"));
    break;
  case driver::StationStatus::  GOT_IP:
    ret.emplace(F("STATION_GOT_IP"));
    break;
  }
  if (!ret.has_value())
    this->logger.error(iop::StaticString(F("Unknown status: ")).toString() + std::to_string(static_cast<uint8_t>(status)));
  return ret;
}