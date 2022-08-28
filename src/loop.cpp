#include "iop/loop.hpp" 
#include "iop-hal/panic.hpp"
#include "iop-hal/wifi.hpp"
#include "iop-hal/io.hpp"
#include "iop/utils.hpp"

#define STRINGIFY(s) STRINGIFY_(s)
#define STRINGIFY_(s) #s

#ifdef IOP_WIFI_SSID
const static char* wifiSSIDRaw = STRINGIFY(IOP_WIFI_SSID);
const static std::optional<iop::StaticString> wifiSSID(reinterpret_cast<const __FlashStringHelper*>(wifiSSIDRaw));
#else
const static std::optional<iop::StaticString> wifiSSID = std::nullopt;
#endif

#ifdef IOP_WIFI_PSK
const static char* wifiPSKRaw = STRINGIFY(IOP_WIFI_PSK);
const static std::optional<iop::StaticString> wifiPSK(reinterpret_cast<const __FlashStringHelper*>(wifiPSKRaw));
#else
const static std::optional<iop::StaticString> wifiPSK = std::nullopt;
#endif

#ifdef IOP_USERNAME
const static char* iopUsernameRaw = STRINGIFY(IOP_USERNAME);
const static std::optional<iop::StaticString> iopUsername(reinterpret_cast<const __FlashStringHelper*>(iopUsernameRaw));
#else
const static std::optional<iop::StaticString> iopUsername = std::nullopt;
#endif

#ifdef IOP_PASSWORD
const static char* iopPasswordRaw = STRINGIFY(IOP_PASSWORD);
const static std::optional<iop::StaticString> iopPassword(reinterpret_cast<const __FlashStringHelper*>(iopPasswordRaw));
#else
const static std::optional<iop::StaticString> iopPassword = std::nullopt;
#endif

namespace iop {
#ifdef IOP_DEBUG
static const char* uriRaw IOP_ROM = "http://127.0.0.1:4001";
#else
static const char* uriRaw IOP_ROM = "https://api.internet-of-plants.org:4001";
#endif
static const StaticString uri(reinterpret_cast<const __FlashStringHelper*>(uriRaw));

EventLoop eventLoop(uri, IOP_LOG_LEVEL);

auto EventLoop::setup() noexcept -> void {
  IOP_TRACE();

  this->logger().info(IOP_STR("Start Setup"));
  //iop_hal::gpio.setMode(iop_hal::io::LED_BUILTIN, iop_hal::io::Mode::OUTPUT);

  iop::Log::setup(IOP_LOG_LEVEL);
  Storage::setup();
  this->api().setup();
  iop::panic::setup();
  iop::network_logger::setup();
  this->credentialsServer.setup();
  this->logger().info(IOP_STR("MD5: "), iop::to_view(iop_hal::device.firmwareMD5()));
  this->logger().info(IOP_STR("Core setup finished, running user layer's setup"));

  iop::setup(*this);
}

auto EventLoop::setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void {
  this->credentialsServer.setAccessPointCredentials(SSID, PSK);
}

AuthenticatedTaskInterval::AuthenticatedTaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept:
  interval(interval), next(0), func(func) {}
TaskInterval::TaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept:
  interval(interval), next(0), func(func) {}

auto EventLoop::setAuthenticatedInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept -> void {
  this->authenticatedTasks.push_back(AuthenticatedTaskInterval(interval, func));
}
auto EventLoop::setInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept -> void {
  this->tasks.push_back(TaskInterval(interval, func));
}

constexpr static uint64_t intervalTryStorageWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr static uint64_t intervalTryHardcodedIopCredentialsMillis =
    60 * 60 * 1000; // 1 hour

auto EventLoop::loop() noexcept -> void {
  this->logger().trace(IOP_STR("\n\n\n\n\n\n"));

  IOP_TRACE();

#ifdef LOG_MEMORY
  iop::logMemory(this->logger());
#endif

  const auto authToken = this->storage().token();
  this->handleInterrupts(authToken);

  const auto now = iop_hal::thisThread.timeRunning();
  const auto isConnected = iop::Network::isConnected();
  this->logger().debug(IOP_STR("Connected: "), std::to_string(isConnected));
  this->logger().debug(IOP_STR("Token: "), std::to_string(authToken.has_value()));

  if (isConnected && authToken) {
    this->credentialsServer.close();
  }

  if (isConnected && this->nextNTPSync < now) {
    this->logger().debug(IOP_STR("Syncing NTP"));
    iop_hal::device.syncNTP();
    this->logger().info(IOP_STR("Time synced"));

    constexpr const uint32_t oneDay = 24 * 60 * 60 * 1000;
    this->nextNTPSync = now + oneDay;
  }

  if (isConnected && !authToken) {
    this->handleIopCredentials();

  } else if (!isConnected) {
    this->handleNotConnected();

  } else {
    this->nextHandleConnectionLost = 0;
    iop_assert(authToken, IOP_STR("Auth Token not found"));
    for (auto& task: this->authenticatedTasks) {
      if (task.next < now) {
        task.next = now + task.interval;
        task.func(*this, authToken->get());
        iop_hal::thisThread.yield();
      }
    }
  }

  for (auto& task: this->tasks) {
    if (task.next < now) {
      task.next = now + task.interval;
      task.func(*this);
      iop_hal::thisThread.yield();
    }
  }
}

auto EventLoop::handleNotConnected() noexcept -> void {
  IOP_TRACE();

  // If connection is lost frequently we open the credentials server, to
  // allow replacing the wifi credentials. Since we only remove it
  // from storage if it's going to be replaced by a new one (allows
  // for more resiliency) - or during factory reset
  constexpr const uint32_t oneMinute = 60 * 1000;

  const auto now = iop_hal::thisThread.timeRunning();
  const auto &wifi = this->storage().wifi();
  const auto isConnected = iop::Network::isConnected();

  if (!isConnected && wifi && this->nextTryStorageWifiCredentials <= now) {
    const auto &stored = wifi->get();
    const auto ssid = iop::to_view(stored.ssid.get());
    const auto psk = iop::to_view(stored.password.get());

    this->logger().info(IOP_STR("Trying wifi credentials stored in storage: "), iop::to_view(iop::scapeNonPrintable(ssid)));
    this->logger().debug(IOP_STR("Password:"), iop::to_view(iop::scapeNonPrintable(psk)));
    switch (this->connect(ssid, psk)) {
      case ConnectResponse::FAIL:
        // WiFi Credentials stored in persistent memory
        //
        // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
        // So we don't delete it and prefer, retrying later
        this->nextTryStorageWifiCredentials = now + intervalTryStorageWifiCredentialsMillis;
        break;
      case ConnectResponse::OK:
      case ConnectResponse::TIMEOUT:
        break;
    }

  } else if (!isConnected && wifiSSID && wifiPSK && this->nextTryHardcodedWifiCredentials <= now) { 
    this->logger().info(IOP_STR("Trying hardcoded wifi credentials"));

    const auto ssid = *wifiSSID;
    const auto psk = *wifiPSK;
    switch (this->connect(ssid.toString(), psk.toString())) {
      case ConnectResponse::FAIL:
      // WiFi Credentials hardcoded
      //
      // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
      // So we keep retrying
        this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
        break;
      case ConnectResponse::OK:
      case ConnectResponse::TIMEOUT:
        break;
    }

  } else {
  //} else if (this->nextHandleConnectionLost < now) {
    // If network is offline for a while we open the captive portal to collect new wifi credentials
    //this->logger().debug(IOP_STR("Has creds, but no signal, opening server"));
    this->nextHandleConnectionLost = now + oneMinute;
    this->handleCredentials();

  //} else {
    // No-op, we must just wait
  }
}

auto EventLoop::handleIopCredentials() noexcept -> void {
  IOP_TRACE();

  const auto now = iop_hal::thisThread.timeRunning();

  if (iopUsername && iopPassword && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;

    this->logger().info(IOP_STR("Trying hardcoded iop credentials"));

    const auto email = *iopUsername;
    const auto password = *iopPassword;
    const auto tok = this->authenticate(email.toString(), password.toString(), this->api());
    if (tok)
      this->storage().setToken(*tok);
  } else {
    this->handleCredentials();
  }
}

auto EventLoop::handleInterrupts(const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept -> void {
  IOP_TRACE();

  while (true) {
    auto ev = iop::descheduleInterrupt();
    if (ev == InterruptEvent::NONE)
      break;

    this->handleInterrupt(ev, token);
    iop_hal::thisThread.yield();
  }
}

auto EventLoop::handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept -> void {
    // Satisfies linter when all interrupt features are disabled
    (void)*this;
    (void)event;
    (void)token;

    IOP_TRACE();
    this->logger().debug(IOP_STR("Handling interrupt: "), std::to_string(static_cast<uint8_t>(event)));

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::MUST_UPGRADE:
      if (token) {
        const auto status = this->api().update(*token);
        switch (status) {
        case iop_hal::UpdateStatus::UNAUTHORIZED:
          this->logger().warn(IOP_STR("Invalid auth token, but keeping since at OTA"));
          return;

        case iop_hal::UpdateStatus::BROKEN_CLIENT:
          iop_panic(IOP_STR("Api::update internal buffer overflow"));

        // Already logged at the network level
        case iop_hal::UpdateStatus::IO_ERROR:
        case iop_hal::UpdateStatus::BROKEN_SERVER:
          // Nothing to be done besides retrying later

        case iop_hal::UpdateStatus::NO_UPGRADE: // Shouldn't happen but ok
          return;
        }

        this->logger().error(IOP_STR("Bad status, EventLoop::handleInterrupt"));
      } else {
        this->logger().error(IOP_STR("Update expected, but no auth token available"));
      }
      break;
    case InterruptEvent::ON_CONNECTION:
      const auto status = iop_hal::statusToString(iop::wifi.status());
      this->logger().debug(IOP_STR("WiFi connected: "), status.value_or(IOP_STR("BadData")));

      const auto [ssid, psk] = iop::wifi.credentials();

      this->storage().setWifi(WifiCredentials(ssid, psk));
      this->logger().info(IOP_STR("Connected to network: "), iop::to_view(iop::scapeNonPrintable(iop::to_view(ssid))));
      break;
    };
}

auto EventLoop::handleCredentials() noexcept -> void {
    IOP_TRACE();

    const auto token = this->credentialsServer.serve(this->api());
    if (token)
      this->storage().setToken(*token);
}

auto EventLoop::connect(std::string_view ssid, std::string_view password) const noexcept -> ConnectResponse {
  IOP_TRACE();
  this->logger().info(IOP_STR("Connect: "), ssid);

  if (!iop::wifi.connectToAccessPoint(ssid, password)) {
    this->logger().error(IOP_STR("Wifi authentication timed out"));
    return ConnectResponse::TIMEOUT;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::wifi.status();
    auto statusStr = iop_hal::statusToString(status);
    if (!statusStr)
      return ConnectResponse::FAIL; // It already will be logged by statusToString;

    this->logger().error(IOP_STR("Invalid wifi credentials ("), *statusStr, IOP_STR("): "), ssid);
  }
  return ConnectResponse::OK;
}

auto EventLoop::registerEvent(const AuthToken& token, const Api::Json json) const noexcept -> void {
  const auto status = this->api().registerEvent(token, json);
  switch (status) {
  case iop::NetworkStatus::BROKEN_CLIENT:
    this->logger().error(IOP_STR("Unable to send measurements"));
    iop_panic(IOP_STR("EventLoop::registerEvent buffer overflow"));

  case iop::NetworkStatus::UNAUTHORIZED:
    this->logger().error(IOP_STR("Unable to send measurements"));
    this->logger().warn(IOP_STR("Auth token was refused, deleting it"));
    this->storage().removeToken();
    return;

  // Already logged at the Network level
  case iop::NetworkStatus::BROKEN_SERVER:
  case iop::NetworkStatus::IO_ERROR:
    // Nothing to be done besides retrying later

  case iop::NetworkStatus::OK: // Cool beans
    return;
  }
  this->logger().error(IOP_STR("Unexpected status at EventLoop::registerEvent"));
}

auto EventLoop::authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::unique_ptr<AuthToken> {
  IOP_TRACE();

  iop::wifi.setMode(iop_hal::WiFiMode::STATION);
  auto authToken = api.authenticate(username, password);
  //iop::wifi.setMode(iop_hal::WiFiMode::ACCESS_POINT_AND_STATION);

  this->logger().debug(IOP_STR("Tried to authenticate"));
  if (const auto *error = std::get_if<iop::NetworkStatus>(&authToken)) {
    const auto &status = *error;

    switch (status) {
    case iop::NetworkStatus::UNAUTHORIZED:
      this->logger().error(IOP_STR("Invalid IoP credentials: "), username);
      return nullptr;

    case iop::NetworkStatus::BROKEN_CLIENT:
      iop_panic(IOP_STR("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::IO_ERROR:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return nullptr;

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(IOP_STR("Unreachable"));
    }

    this->logger().crit(IOP_STR("CredentialsServer::authenticate bad status"));
    return nullptr;

  } else if (auto *token = std::get_if<std::unique_ptr<AuthToken>>(&authToken)) {
    return std::move(*token);
  }

  iop_panic(IOP_STR("Invalid variant"));
}
}
