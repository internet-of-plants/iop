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
#ifdef IOP_POSIX
static const char* uriRaw IOP_ROM = "http://localhost:4001";
#else
static const char* uriRaw IOP_ROM = "https://iop-monitor-server.tk:4001";
#endif
static const StaticString uri(reinterpret_cast<const __FlashStringHelper*>(uriRaw));

EventLoop eventLoop(uri, IOP_LOG_LEVEL);

auto EventLoop::setup() noexcept -> void {
  iop::panic::setup();
  iop::network_logger::setup();

  IOP_TRACE();

  this->logger().info(IOP_STR("Start Setup"));
  iop_hal::gpio.setMode(iop_hal::io::LED_BUILTIN, iop_hal::io::Mode::OUTPUT);

  Storage::setup();
  this->api().setup();
  this->credentialsServer.setup();
  this->logger().info(IOP_STR("Core setup finished, running user layer's setup"));
  this->logger().info(IOP_STR("MD5: "), iop::to_view(iop_hal::device.firmwareMD5()));

  iop::setup(*this);
}

TaskInterval::TaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept:
  interval(interval), next(0), func(func) {}
AuthenticatedTaskInterval::AuthenticatedTaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, AuthToken&)> func) noexcept:
  interval(interval), next(0), func(func) {}

auto EventLoop::setAuthenticatedInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, AuthToken&)> func) noexcept -> void {
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

  // Handle all queued interrupts (only allows one of each kind concurrently)
  while (true) {
      auto ev = iop::descheduleInterrupt();
      if (ev == InterruptEvent::NONE)
        break;

      this->handleInterrupt(ev, authToken);
      iop_hal::thisThread.yield();
  }
  const auto now = iop_hal::thisThread.timeRunning();
  const auto isConnected = iop::Network::isConnected();

  if (isConnected && authToken)
      this->credentialsServer.close();

  if (isConnected && this->nextNTPSync < now) {
    this->logger().info(IOP_STR("Syncing NTP"));

    iop_hal::device.syncNTP();
    
    this->logger().info(IOP_STR("Time synced"));

    constexpr const uint32_t oneDay = 24 * 60 * 60 * 1000;
    this->nextNTPSync = now + oneDay;

  } else if (isConnected && !authToken) {
      this->handleIopCredentials();

  } else if (!isConnected) {
      this->handleNotConnected();

  } else {
    this->nextHandleConnectionLost = 0;
    iop_assert(authToken, IOP_STR("Auth Token not found"));
    for (auto& task: this->authenticatedTasks) {
      if (task.next < now) {
        task.next = now + task.interval;
        task.func(*this, *authToken);
      }
    }
  }

  for (auto& task: this->tasks) {
    if (task.next < now) {
      task.next = now + task.interval;
      task.func(*this);
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
    this->connect(ssid, psk);

    // WiFi Credentials stored in persistent memory
    //
    // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
    // So we don't delete it and prefer, retrying later
    this->nextTryStorageWifiCredentials = now + intervalTryStorageWifiCredentialsMillis;

  } else if (!isConnected && wifiSSID && wifiPSK && this->nextTryHardcodedWifiCredentials <= now) { 
    this->logger().info(IOP_STR("Trying hardcoded wifi credentials"));

    const auto ssid = *wifiSSID;
    const auto psk = *wifiPSK;
    this->connect(ssid.toString(), psk.toString());

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but we can't know if it's wrong or if the router just is offline
    // So we keep retrying
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;

  } else if (this->nextHandleConnectionLost < now) {
    // If network is offline for a while we open the captive portal to collect new wifi credentials
    this->logger().debug(IOP_STR("Has creds, but no signal, opening server"));
    this->nextHandleConnectionLost = now + oneMinute;
    this->handleCredentials();

  } else {
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

auto EventLoop::handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept -> void {
    // Satisfies linter when all interrupt features are disabled
    (void)*this;
    (void)event;
    (void)token;

    IOP_TRACE();

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::MUST_UPGRADE:
      if (token) {
        const auto status = this->api().upgrade(*token);
        switch (status) {
        case iop_hal::UpgradeStatus::FORBIDDEN:
          this->logger().warn(IOP_STR("Invalid auth token, but keeping since at OTA"));
          return;

        case iop_hal::UpgradeStatus::BROKEN_CLIENT:
          iop_panic(IOP_STR("Api::upgrade internal buffer overflow"));

        // Already logged at the network level
        case iop_hal::UpgradeStatus::IO_ERROR:
        case iop_hal::UpgradeStatus::BROKEN_SERVER:
          // Nothing to be done besides retrying later

        case iop_hal::UpgradeStatus::NO_UPGRADE: // Shouldn't happen but ok
          return;
        }

        this->logger().error(IOP_STR("Bad status, EventLoop::handleInterrupt"));
      } else {
        this->logger().error(IOP_STR("Upgrade expected, but no auth token available"));
      }
      break;
    case InterruptEvent::ON_CONNECTION:
      const auto status = iop_hal::statusToString(iop::wifi.status());
      this->logger().debug(IOP_STR("WiFi connected: "), status.value_or(IOP_STR("BadData")));

      const auto [ssid, psk] = iop::wifi.credentials();

      this->logger().info(IOP_STR("Connected to network: "), iop::to_view(iop::scapeNonPrintable(iop::to_view(ssid))));
      this->storage().setWifi(WifiCredentials(ssid, psk));
      break;
    };
}

auto EventLoop::handleCredentials() noexcept -> void {
    IOP_TRACE();

    const auto token = this->credentialsServer.serve(this->api());
    if (token)
      this->storage().setToken(*token);
}

auto EventLoop::connect(std::string_view ssid, std::string_view password) const noexcept -> void {
  IOP_TRACE();
  this->logger().info(IOP_STR("Connect: "), ssid);

  if (!iop::wifi.connectToAccessPoint(ssid, password)) {
    this->logger().error(IOP_STR("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::wifi.status();
    auto statusStr = iop_hal::statusToString(status);
    if (!statusStr)
      return; // It already will be logged by statusToString;

    this->logger().error(IOP_STR("Invalid wifi credentials ("), *statusStr, IOP_STR("): "), std::move(ssid));
  }
}

auto EventLoop::authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::optional<AuthToken> {
  IOP_TRACE();

  iop::wifi.setMode(iop_hal::WiFiMode::STATION);
  auto authToken = api.authenticate(username, std::move(password));
  iop::wifi.setMode(iop_hal::WiFiMode::ACCESS_POINT_AND_STATION);

  this->logger().info(IOP_STR("Tried to authenticate"));
  if (const auto *error = std::get_if<iop::NetworkStatus>(&authToken)) {
    const auto &status = *error;

    switch (status) {
    case iop::NetworkStatus::FORBIDDEN:
      this->logger().error(IOP_STR("Invalid IoP credentials: "), username);
      return std::nullopt;

    case iop::NetworkStatus::BROKEN_CLIENT:
      iop_panic(IOP_STR("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::IO_ERROR:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return std::nullopt;

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(IOP_STR("Unreachable"));
    }

    this->logger().crit(IOP_STR("CredentialsServer::authenticate bad status"));
    return std::nullopt;

  } else if (auto *token = std::get_if<AuthToken>(&authToken)) {
    return *token;
  }

  iop_panic(IOP_STR("Invalid variant"));
}
}