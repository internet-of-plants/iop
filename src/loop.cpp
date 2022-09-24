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

  this->logger().infoln(IOP_STR("Start Setup"));
  //iop_hal::gpio.setMode(iop_hal::io::LED_BUILTIN, iop_hal::io::Mode::OUTPUT);

  iop::Log::setup(IOP_LOG_LEVEL);
  Storage::setup();
  this->api().setup();
  iop::panic::setup();
  iop::network_logger::setup();
  this->credentialsServer.setup();
  const auto md5 = iop::to_view(iop_hal::device.firmwareMD5());
  this->logger().info(IOP_STR("MD5: "));
  this->logger().infoln(md5);
  this->logger().infoln(IOP_STR("Core setup finished, running user layer's setup"));

  iop::setup(*this);
}

auto EventLoop::setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void {
  this->credentialsServer.setAccessPointCredentials(SSID, PSK);
}

AuthenticatedTaskInterval::AuthenticatedTaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept:
  next(0), interval(interval), func(func) {}
TaskInterval::TaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept:
  next(0), interval(interval), func(func) {}

auto EventLoop::setAuthenticatedInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept -> void {
  this->authenticatedTasks.push_back(AuthenticatedTaskInterval(interval, func));
}
auto EventLoop::setInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept -> void {
  this->tasks.push_back(TaskInterval(interval, func));
}

constexpr static uint64_t intervalTryStorageWifiCredentialsMillis =
    10 * 60 * 1000; // 10 minutes

constexpr static uint64_t intervalTryHardcodedWifiCredentialsMillis =
    10 * 60 * 1000; // 10 minutes

constexpr static uint64_t intervalTryHardcodedIopCredentialsMillis =
    10 * 60 * 1000; // 10 minutes

auto EventLoop::syncNTP() noexcept -> void {
  IOP_TRACE();

  this->logger().debugln(IOP_STR("Syncing NTP"));
  iop_hal::device.syncNTP();
  this->logger().infoln(IOP_STR("Time synced"));

  constexpr const uint32_t oneDay = 24 * 60 * 60 * 1000;
  this->nextNTPSync = iop_hal::thisThread.timeRunning() + oneDay;
}

auto EventLoop::runAuthenticatedTasks() noexcept -> void {
  IOP_TRACE();

  const auto token = this->storage().token();
  iop_assert(token, IOP_STR("Auth Token not found"));

  for (auto & task: this->authenticatedTasks) {
    if (task.next < iop_hal::thisThread.timeRunning()) {
      task.next = iop_hal::thisThread.timeRunning() + task.interval;
      (task.func)(*this, *token);
      iop_hal::thisThread.yield();
    }
  }
}

auto EventLoop::runUnauthenticatedTasks() noexcept -> void {
  IOP_TRACE();

  for (auto & task: this->tasks) {
    if (task.next < iop_hal::thisThread.timeRunning()) {
      task.next = iop_hal::thisThread.timeRunning() + task.interval;
      (task.func)(*this);
      iop_hal::thisThread.yield();
    }
  }
}

auto EventLoop::logIteration() noexcept -> void {
  this->logger().traceln(IOP_STR("\n\n\n\n\n\n"));
  iop::logMemory(this->logger());

  this->logger().trace(IOP_STR("Has Wifi Creds: "));
  this->logger().trace(std::to_string(this->storage().wifi().has_value()));

  this->logger().trace(IOP_STR("Has IoP Creds: "));
  this->logger().trace(std::to_string(this->storage().token().has_value()));
}

auto EventLoop::loop() noexcept -> void {
  this->logIteration();

  if (this->handleInterrupts()) {
    return;
  }
  
  if (iop::Network::isConnected() && this->storage().token()) {
    this->credentialsServer.close();
  }

  if (iop::Network::isConnected() && this->nextNTPSync < iop_hal::thisThread.timeRunning()) {
    this->syncNTP();

  } else if (iop::Network::isConnected() && !this->storage().token()) {
    this->handleIopCredentials();

  } else if (!iop::Network::isConnected()) {
    this->handleNotConnected();

  } else {
    this->runAuthenticatedTasks();
  }

  this->runUnauthenticatedTasks();
}

auto EventLoop::handleStoredWifiCreds() noexcept -> void {
  IOP_TRACE();

  const auto &wifi = this->storage().wifi();
  if (!wifi) return;

  const WifiCredentials &stored = wifi->get();
  const auto ssid = iop::to_view(stored.ssid.get());
  const auto psk = iop::to_view(stored.password.get());

  this->logger().info(IOP_STR("Trying wifi credentials stored in storage: "));
  this->logger().infoln(iop::scapeNonPrintable(ssid));
  this->logger().debug(IOP_STR("Password:"));
  this->logger().debugln(iop::scapeNonPrintable(psk));
  switch (this->connect(ssid, psk)) {
    case ConnectResponse::OK:
      break;
    case ConnectResponse::TIMEOUT:
      // WiFi Credentials stored in persistent memory
      //
      // Ideally credentials be wrong, but we can't know if it's wrong or if it just timed-out or the router is offline
      // So we keep retrying
      this->nextTryStorageWifiCredentials = iop_hal::thisThread.timeRunning() + intervalTryStorageWifiCredentialsMillis;
      break;
  }
}

auto EventLoop::handleHardcodedWifiCreds() noexcept -> void {
  IOP_TRACE();
  this->logger().infoln(IOP_STR("Trying hardcoded wifi credentials"));

  const auto ssid = *wifiSSID;
  const auto psk = *wifiPSK;
  switch (this->connect(ssid.toString(), psk.toString())) {
    case ConnectResponse::OK:
      break;
    case ConnectResponse::TIMEOUT:
      // WiFi Credentials hardcoded
      //
      // Ideally credentials won't be wrong, but we can't know if it's wrong or if it just timed-out or the router is offline
      // So we keep retrying
      this->nextTryHardcodedWifiCredentials = iop_hal::thisThread.timeRunning() + intervalTryHardcodedWifiCredentialsMillis;
      break;
  }
}

auto EventLoop::handleNotConnected() noexcept -> void {
  // If connection is lost and all stored creds have been tried we open the credentials server,
  // to allow for replacing the wifi credentials if they are wrong. Since we can't know if they are wrong
  // or it's timing out, or the router is offline.
  //
  // But hardcoded or creds persisted in memory will be retried

  if (!iop::Network::isConnected() && this->storage().wifi() && this->nextTryStorageWifiCredentials <= iop_hal::thisThread.timeRunning()) {
    this->handleStoredWifiCreds();

  } else if (!iop::Network::isConnected() && wifiSSID && wifiPSK && this->nextTryHardcodedWifiCredentials <= iop_hal::thisThread.timeRunning()) { 
    this->handleHardcodedWifiCreds();

  } else {
    this->handleCredentials();
  }
}

auto EventLoop::handleHardcodedIopCreds() noexcept -> void {
  IOP_TRACE();

  if (iopUsername && iopPassword) {
    this->nextTryHardcodedIopCredentials = iop_hal::thisThread.timeRunning() + intervalTryHardcodedIopCredentialsMillis;

    this->logger().infoln(IOP_STR("Trying hardcoded iop credentials"));

    const auto tok = this->authenticate(iopUsername->toString(), iopPassword->toString(), this->api());
    if (tok)
      this->storage().setToken(*tok);
  }
}

auto EventLoop::handleIopCredentials() noexcept -> void {
  if (iopUsername && iopPassword && this->nextTryHardcodedIopCredentials <= iop_hal::thisThread.timeRunning()) {
    this->handleHardcodedIopCreds();
  } else {
    this->handleCredentials();
  }
}

auto EventLoop::handleInterrupts() noexcept -> bool {
  IOP_TRACE();

  const auto authToken = this->storage().token();
  if (!authToken) return false;

  auto handled = false;
  while (true) {
    auto ev = iop::descheduleInterrupt();
    if (ev == InterruptEvent::NONE)
      return handled;

    handled = true;
    this->handleInterrupt(ev, *authToken);
    iop_hal::thisThread.yield();
  }
}

auto upgrade(EventLoop & loop, const std::optional<std::reference_wrapper<const AuthToken>> &token) noexcept -> void {
  if (token) {
    const auto status = loop.api().update(*token);
    switch (status) {
    case iop_hal::UpdateStatus::UNAUTHORIZED:
      loop.logger().warnln(IOP_STR("Invalid auth token, but keeping since at OTA"));
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

    loop.logger().errorln(IOP_STR("Bad status, EventLoop::handleInterrupt"));
  } else {
    loop.logger().errorln(IOP_STR("Update expected, but no auth token available"));
  }
}

auto onConnect(EventLoop & loop) noexcept -> void {
  IOP_TRACE();

  const auto status = iop_hal::statusToString(iop::wifi.status());
  const auto [ssid, psk] = iop::wifi.credentials();

  loop.logger().info(IOP_STR("Connected to network: "));
  loop.logger().info(iop::scapeNonPrintable(iop::to_view(ssid)));
  loop.logger().info(IOP_STR(", status: "));
  loop.logger().infoln(status);

  loop.storage().setWifi(WifiCredentials(ssid, psk));
}

auto EventLoop::handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) noexcept -> void {
    // Satisfies linter when all interrupt features are disabled
    (void)*this;
    (void)event;
    (void)token;

    IOP_TRACE();
    this->logger().debug(IOP_STR("Handling interrupt: "));
    this->logger().debugln(static_cast<uint64_t>(event));

    switch (event) {
    case InterruptEvent::NONE:
      break;
    case InterruptEvent::MUST_UPGRADE:
      upgrade(*this, token);
      break;
    case InterruptEvent::ON_CONNECTION:
      onConnect(*this);
      break;
    };
}

auto EventLoop::handleCredentials() noexcept -> void {
    IOP_TRACE();

    const auto token = this->credentialsServer.serve(this->api());
    if (token)
      this->storage().setToken(*token);
}

auto EventLoop::connect(std::string_view ssid, std::string_view password) noexcept -> ConnectResponse {
  this->logger().info(IOP_STR("Connect: "));
  this->logger().infoln(iop::scapeNonPrintable(iop::to_view(ssid)));

  if (!iop::wifi.connectToAccessPoint(ssid, password)) {
    this->logger().errorln(IOP_STR("Wifi authentication timed out"));
    return ConnectResponse::TIMEOUT;
  }

  if (!iop::Network::isConnected()) {
    const auto status = iop::wifi.status();
    auto statusStr = iop_hal::statusToString(status);

    this->logger().error(IOP_STR("Invalid wifi credentials ("));
    this->logger().error(statusStr);
    this->logger().error(IOP_STR("): "));
    this->logger().errorln(iop::scapeNonPrintable(iop::to_view(ssid)));
  } else {
    this->logger().infoln(IOP_STR("Connected to WiFi"));
  }
  return ConnectResponse::OK;
}

auto EventLoop::registerEvent(const AuthToken& token, const Api::Json json) noexcept -> void {
  const auto status = this->api().registerEvent(token, json);
  switch (status) {
  case iop::NetworkStatus::BROKEN_CLIENT:
    this->logger().errorln(IOP_STR("Unable to send measurements"));
    iop_panic(IOP_STR("EventLoop::registerEvent buffer overflow"));

  case iop::NetworkStatus::UNAUTHORIZED:
    this->logger().errorln(IOP_STR("Unable to send measurements"));
    this->logger().warnln(IOP_STR("Auth token was refused, deleting it"));
    this->storage().removeToken();
    return;

  // Already logged at the Network level
  case iop::NetworkStatus::BROKEN_SERVER:
  case iop::NetworkStatus::IO_ERROR:
    // Nothing to be done besides retrying later

  case iop::NetworkStatus::OK: // Cool beans
    return;
  }
  this->logger().errorln(IOP_STR("Unexpected status at EventLoop::registerEvent"));
}

auto EventLoop::authenticate(std::string_view username, std::string_view password, Api &api) noexcept -> std::unique_ptr<AuthToken> {
  iop::wifi.setMode(iop_hal::WiFiMode::STATION);
  auto authToken = api.authenticate(username, password);
  //iop::wifi.setMode(iop_hal::WiFiMode::ACCESS_POINT_AND_STATION);

  this->logger().debugln(IOP_STR("Tried to authenticate"));
  if (const auto *error = std::get_if<iop::NetworkStatus>(&authToken)) {
    const auto &status = *error;

    switch (status) {
    case iop::NetworkStatus::UNAUTHORIZED:
      this->logger().error(IOP_STR("Invalid IoP credentials: "));
      this->logger().errorln(username);
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

    this->logger().critln(IOP_STR("CredentialsServer::authenticate bad status"));
    return nullptr;

  } else if (auto *token = std::get_if<std::unique_ptr<AuthToken>>(&authToken)) {
    return std::move(*token);
  }

  iop_panic(IOP_STR("Invalid variant"));
}
}
