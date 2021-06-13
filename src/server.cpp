#include "server.hpp"

#ifndef IOP_SERVER_DISABLED

#include "core/utils.hpp"
#include "driver/server.hpp"
#include "driver/wifi.hpp"
#include "driver/thread.hpp"

#include "configuration.hpp"
#include "driver/interrupt.hpp"
#include "core/network.hpp"
#include "flash.hpp"
#include "api.hpp"
#include "driver/device.hpp"

constexpr const uint64_t intervalTryFlashWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr const uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr const uint64_t intervalTryHardcodedIopCredentialsMillis =
    60 * 60 * 1000; // 1 hour

auto pageHTMLStart() -> iop::StaticString {
  return iop::StaticString(F(
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h4><center>If, in the future, you want to reset the "
    "configurations set here, just press the factory reset button "
    "for at least 15 seconds</center></h4>"
    "<form style='margin: 0 auto; width: 500px;' action='/submit' "
    "method='POST'>\r\n"));
}

auto wifiOverwriteHTML() -> iop::StaticString {
  return iop::StaticString(F(
    "<h3>"
    "  <center>It seems you already have your wifi credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center>"
    "</h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='wifi'>"
    "  <label for='wifi'>Overwrite wifi credentials</label>"
    "</div>"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"wifi\" style=\"display: none\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='password' type='password' style='width:100%' />"
    "</div>\r\n"));
}

auto wifiHTML() -> iop::StaticString {
  return iop::StaticString(F(
    "<h3><center>"
    "Please provide your Wifi credentials, so we can connect to it."
    "</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='wifi'></div>"
    "<div>"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div><div><strong>Password:</strong></div>"
    "<input name='password' type='password' style='width:100%' /></div>\r\n"));
}

auto iopOverwriteHTML() -> iop::StaticString {
  return iop::StaticString(F(
    "<h3><center>It seems you already have your Iop credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center></h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='iop'>"
    "  <label for='iop'>Overwrite Iop credentials</label>"
    "</div>"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"iop\" style=\"display: 'none'\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n"));
}

auto iopHTML() -> iop::StaticString {
  return iop::StaticString(F(
    "<h3><center>Please provide your Iop credentials, so we can get an "
    "authentication token to use</center></h3>\r\n"
    "<div>"
    "  <input type='hidden' value='true' name='iop'></div>"
    "<div>"
    "  <div><strong>Email:</strong></div>"
    "  <input name='iopEmail' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div>"
    "  <div><strong>Password:</strong></div>"
    "  <input name='iopPassword' type='password' style='width:100%' />"
    "</div>\r\n"));
}

auto script() -> iop::StaticString {
  return iop::StaticString(F(
    "<script type='application/javascript'>"
    "document.querySelector(\"input[name='wifi']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('wifi')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "document.querySelector(\"input[name='iop']\").addEventListener('change', ev => {"
    "  for (const el of document.getElementsByClassName('iop')) {"
    "    if (ev.currentTarget.checked) {"
    "      el.style.display = 'block';"
    "    } else {"
    "      el.style.display = 'none';"
    "    }"
    "  }"
    "});"
    "</script>"));
}

auto pageHTMLEnd() -> iop::StaticString {
  return iop::StaticString(F(
    "<br>\r\n"
    "<input type='submit' value='Submit' />\r\n"
    "</form></body></html>"));
}

// We use this globals to share messages from the callbacks
static std::optional<std::pair<std::string, std::string>> credentialsWifi;
static std::optional<std::pair<std::string, std::string>> credentialsIop;

static driver::HttpServer server;
//static ESP8266WebServer server;

static driver::CaptivePortal dnsServer;

#include <iostream>

void CredentialsServer::setup() const noexcept {
  IOP_TRACE();
  // Self reference, but it's to a static
  server.on(F("/favicon.ico"), [](driver::HttpConnection &conn, iop::Log const &logger) { conn.send(HTTP_CODE_NOT_FOUND, F("text/plain"), F("")); (void) logger; });
  server.on(F("/submit"), [](driver::HttpConnection &conn, iop::Log const &logger) {
    IOP_TRACE();
    logger.debug(F("Received credentials form"));

    const auto wifi = conn.arg(F("wifi"));
    const auto maybeSsid = conn.arg(F("ssid"));
    const auto maybePsk = conn.arg(F("password"));
    if (wifi.has_value() && maybeSsid.has_value() && maybePsk.has_value()) {
      const auto &ssid = iop::unwrap_ref(maybeSsid, IOP_CTX());
      const auto &psk = iop::unwrap_ref(maybePsk, IOP_CTX());
      logger.debug(F("SSID: "), ssid);

      credentialsWifi = std::make_optional(std::make_pair(ssid, psk));
    }

    const auto iop = conn.arg(F("iop"));
    const auto maybeEmail = conn.arg(F("iopEmail"));
    const auto maybePassword = conn.arg(F("iopPassword"));
    if (iop.has_value() && maybeEmail.has_value() && maybePassword.has_value()) {
      const auto &email = iop::unwrap_ref(maybeEmail, IOP_CTX());
      const auto &password = iop::unwrap_ref(maybePassword, IOP_CTX());
      logger.debug(F("Email: "), email);

      credentialsIop = std::make_optional(std::make_pair(email, password));
    }

    conn.sendHeader(F("Location"), F("/"));
    conn.send(HTTP_CODE_FOUND, F("text/plain"), F(""));
  });

  server.onNotFound([](driver::HttpConnection &conn, iop::Log const &logger) {
    IOP_TRACE();
    const static Flash flash(logger.level());
    logger.info(F("Serving captive portal"));

    const auto mustConnect = !iop::Network::isConnected();
    const auto needsIopAuth = !flash.readAuthToken().has_value();

    auto len = pageHTMLStart().length() + pageHTMLEnd().length() + script().length();
    len += mustConnect ? wifiHTML().length() : wifiOverwriteHTML().length();
    len += needsIopAuth ? iopHTML().length() : iopOverwriteHTML().length();

    conn.setContentLength(len);
    conn.send(HTTP_CODE_OK, F("text/html"), pageHTMLStart());

    if (mustConnect) conn.sendData(wifiHTML());
    else conn.sendData(wifiOverwriteHTML());

    if (needsIopAuth) conn.sendData(iopHTML());
    else conn.sendData(iopOverwriteHTML());

    conn.sendData(script());
    conn.sendData(pageHTMLEnd());
    logger.debug(F("Served HTML"));
  });
}

void CredentialsServer::start() noexcept {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.info(F("Setting our own wifi access point"));

    // TODO: how to mock it in a reasonable way?
    WiFi.mode(WIFI_AP_STA);
    driver::thisThread.sleep(1);

    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto staticIp = IPAddress(192, 168, 1, 1);
    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto mask = IPAddress(255, 255, 255, 0);

    WiFi.softAPConfig(staticIp, staticIp, mask);

    const static auto hash = iop::hashString(driver::device.macAddress().asString().borrow());
    const auto ssid = std::string("iop-") + std::to_string(hash);

    // TODO(pc): the password should be random (passed at compile time)
    // But also accessible externally (like a sticker in the hardware).
    // So not dynamic
    //
    // Using PSTR in the password crashes, because it internally calls
    // strlen and it's a hardware fault reading byte by byte from flash
    WiFi.softAP(ssid.c_str(), "le$memester#passwordz");
    
    const auto ip = WiFi.softAPIP().toString();
    
    server.begin();

    // Makes it a captive portal (redirects all wifi trafic to it)
    dnsServer.start();

    this->logger.info(F("Opened captive portal: "), iop::to_view(ip));
  }
}

void CredentialsServer::close() noexcept {
  IOP_TRACE();
  if (this->isServerOpen) {
    this->logger.debug(F("Closing captive portal"));
    this->isServerOpen = false;
    dnsServer.close();
    server.close();

    WiFi.mode(WIFI_STA);
    driver::thisThread.sleep(1);
  }
}

auto CredentialsServer::statusToString(const driver::StationStatus status)
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
    this->logger.error(iop::StaticString(F("Unknown status: ")).toStdString() + std::to_string(static_cast<uint8_t>(status)));
  return ret;
}

auto CredentialsServer::connect(std::string_view ssid,
                                std::string_view password) const noexcept
    -> void {
  IOP_TRACE();
  this->logger.info(F("Connect: "), ssid);
  if (driver::wifi.status() == driver::StationStatus::CONNECTING) {
    const iop::InterruptLock _guard;
    driver::wifi.stationDisconnect();
  }

  WiFi.begin(ssid.begin(), std::move(password).begin());

  if (WiFi.waitForConnectResult() == -1) {
    this->logger.error(F("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = driver::wifi.status();
    auto maybeStatusStr = this->statusToString(status);
    if (!maybeStatusStr.has_value())
      return; // It already will be logged by statusToString;

    const auto statusStr = iop::unwrap(maybeStatusStr, IOP_CTX());
    this->logger.error(F("Invalid wifi credentials ("), statusStr, F("): "), std::move(ssid));
  }
}

auto CredentialsServer::authenticate(std::string_view username,
                                     std::string_view password,
                                     const Api &api) const noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  WiFi.mode(WIFI_STA);

  auto authToken = api.authenticate(username, std::move(password));
  
  WiFi.mode(WIFI_AP_STA);
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

auto CredentialsServer::serve(const std::optional<WifiCredentials> &storedWifi,
                              const Api &api) noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  this->start();

  const auto now = driver::thisThread.now();

  // The user provided those informations through the web form
  // But we shouldn't act on it inside the server's callback, as callback
  // should be rather simple, so we use globals. UNWRAP moves them out on use.

  const auto isConnected = iop::Network::isConnected();

  if (credentialsWifi.has_value()) {
    const auto wifi = iop::unwrap(credentialsWifi, IOP_CTX());
    this->connect(wifi.first, wifi.second);
  }
  
  if (isConnected && credentialsIop.has_value()) {
    const auto iop = iop::unwrap(credentialsIop, IOP_CTX());
    auto tok = this->authenticate(iop.first, iop.second, api);
    if (tok.has_value())
      return tok;

    // WiFi Credentials stored in flash

    // Wifi connection errors are hard to debug
    // (see countless open issues about it at esp8266/Arduino's github)
    // One example citing others:
    // https://github.com/esp8266/Arduino/issues/7432
    //
    // So we never delete a flash stored wifi credentials (outside of factory
    // reset), we have a timer to avoid constantly retrying a bad credential.
  }
  
  if (!isConnected && storedWifi.has_value() && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = iop::unwrap_ref(storedWifi, IOP_CTX());
    this->logger.info(F("Trying wifi credentials stored in flash"));
    this->connect(stored.ssid.asString().get(), stored.password.asString().get());

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but that's the price of hardcoding, if it's
    // not updated it may just be. Since it can't be deleted we must retry,
    // but with a bigish interval.
  }
  
  const auto hasHardcodedWifiCreds = wifiNetworkName().has_value() && wifiPassword().has_value();
  if (!isConnected && hasHardcodedWifiCreds && this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;

    this->logger.info(F("Trying hardcoded wifi credentials"));

    const auto &ssid = iop::unwrap_ref(wifiNetworkName(), IOP_CTX());
    const auto &psk = iop::unwrap_ref(wifiPassword(), IOP_CTX());
    this->connect(ssid.toStdString(), psk.toStdString());
  }

  const auto hasHardcodedIopCreds = iopEmail().has_value() && iopPassword().has_value();
  if (!isConnected && hasHardcodedIopCreds && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;

    this->logger.info(F("Trying hardcoded iop credentials"));

    const auto &email = iop::unwrap_ref(iopEmail(), IOP_CTX());
    const auto &password = iop::unwrap_ref(iopPassword(), IOP_CTX());
    this->authenticate(email.toStdString(), password.toStdString(), api);
  }

  // Give processing time to the servers
  this->logger.trace(F("Serve captive portal"));
  dnsServer.handleClient();
  server.handleClient();
  return std::optional<AuthToken>();
}
#else
  auto CredentialsServer::serve(const std::optional<WifiCredentials> &storedWifi,
             const Api &api) noexcept -> std::optional<AuthToken> {
  IOP_TRACE();
  (void)*this;
  (void)api;
  (void)storedWifi;
  return std::optional<AuthToken>();
}
void CredentialsServer::setup() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void CredentialsServer::close() noexcept {
  (void)*this;
  IOP_TRACE();
}
void CredentialsServer::start() noexcept {
  (void)*this;
  IOP_TRACE();
}
#endif
