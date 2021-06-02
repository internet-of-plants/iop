#include "server.hpp"

#ifndef IOP_SERVER_DISABLED

#include "driver/server.hpp"
#include "driver/wifi.hpp"
#include "driver/time.hpp"

#include "configuration.hpp"
#include "driver/interrupt.hpp"
#include "core/network.hpp"
#include "flash.hpp"
#include "api.hpp"

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

static ESP8266WebServer server;

constexpr const uint8_t dnsPort = 53;
static DNSServer dnsServer;

#include <iostream>

void CredentialsServer::setup() const noexcept {
  IOP_TRACE();
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  const auto loggerLevel = this->logger.level();

  // Self reference, but it's to a static
  auto *s = &server;
  server.on(F("/favicon.ico"), [s]() { s->send_P(HTTP_CODE_NOT_FOUND, PSTR("text/plain"), PSTR("")); });
  server.on(F("/submit"), [s, loggerLevel]() {
    IOP_TRACE();
    if (loggerLevel >= iop::LogLevel::DEBUG)
      iop::Log::print(
          F("[DEBUG] [RAW] SERVER_CALLBACK: Received credentials form\n"),
          iop::LogLevel::DEBUG, iop::LogType::STARTEND);

    if (s->hasArg(F("wifi")) && s->hasArg(F("ssid")) &&
        s->hasArg(F("password"))) {
      const auto ssid = std::string(s->arg(F("ssid")).c_str());
      const auto psk = std::string(s->arg(F("password")).c_str());

      if (loggerLevel >= iop::LogLevel::DEBUG) {
        iop::Log::print(
            F("[DEBUG] [RAW] SERVER_CALLBACK: SSID "),
            iop::LogLevel::DEBUG, iop::LogType::START);
        iop::Log::print(
            ssid.c_str(),
            iop::LogLevel::DEBUG, iop::LogType::CONTINUITY);
        iop::Log::print(
            F("\n"),
            iop::LogLevel::DEBUG, iop::LogType::END);
      }
      if (ssid.length() > 0 && psk.length() > 0)
        credentialsWifi = std::make_optional(std::make_pair(ssid, psk));
    }

    if (s->hasArg(F("iop")) && s->hasArg(F("iopEmail")) &&
        s->hasArg(F("iopPassword"))) {
      const std::string email(s->arg(F("iopEmail")).c_str());
      const std::string password(s->arg(F("iopPassword")).c_str());
      if (email.length() > 0 && password.length() > 0) {
        credentialsIop = std::make_optional(std::make_pair(email, password));
      }
    }

    s->sendHeader(F("Location"), F("/"));
    s->send_P(HTTP_CODE_FOUND, PSTR("text/plain"), PSTR(""));
  });

  server.onNotFound([s, loggerLevel]() {
    IOP_TRACE();
    const static Flash flash(loggerLevel);
    if (loggerLevel >= iop::LogLevel::DEBUG)
      iop::Log::print(
          F("[INFO] [RAW] SERVER_CALLBACK: Serving captive portal HTML\n"),
          iop::LogLevel::DEBUG, iop::LogType::STARTEND);

    const auto mustConnect = !iop::Network::isConnected();
    const auto needsIopAuth = !flash.readAuthToken().has_value();

    auto len = pageHTMLStart().length() + pageHTMLEnd().length() + script().length();

    if (mustConnect) {
      len += wifiHTML().length();
    } else {
      len += wifiOverwriteHTML().length();
    }

    if (needsIopAuth) {
      len += iopHTML().length();
    } else {
      len += iopOverwriteHTML().length();
    }

    s->setContentLength(len);


    s->send_P(HTTP_CODE_OK, PSTR("text/html"), pageHTMLStart().asCharPtr());

    if (mustConnect) {
      s->sendContent_P(wifiHTML().asCharPtr());
    } else {
      s->sendContent_P(wifiOverwriteHTML().asCharPtr());
    }

    if (needsIopAuth) {
      s->sendContent_P(iopHTML().asCharPtr());
    } else {
      s->sendContent_P(iopOverwriteHTML().asCharPtr());
    }

    s->sendContent_P(script().asCharPtr());
    
    s->sendContent_P(pageHTMLEnd().asCharPtr());
    iop::Log::print(F("[INFO] [RAW] SERVER_CALLBACK: Served HTML\n"),
      iop::LogLevel::DEBUG, iop::LogType::STARTEND);
  });
}

void CredentialsServer::start() noexcept {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.info(F("Setting our own wifi access point"));

    // TODO: how to mock it in a reasonable way?
    WiFi.mode(WIFI_AP_STA);
    delay(1);

    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto staticIp = IPAddress(192, 168, 1, 1);
    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto mask = IPAddress(255, 255, 255, 0);

    WiFi.softAPConfig(staticIp, staticIp, mask);

    const static auto hash = iop::macAddress().asString().borrow().hash();
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
    dnsServer.start(dnsPort, F("*"), WiFi.softAPIP());

    this->logger.info(F("Opened captive portal: "), ip);
  }
}

void CredentialsServer::close() noexcept {
  IOP_TRACE();
  if (this->isServerOpen) {
    this->logger.debug(F("Closing captive portal"));
    this->isServerOpen = false;
    dnsServer.stop();
    server.close();

    WiFi.mode(WIFI_STA);
    delay(1);
  }
}

auto CredentialsServer::statusToString(const station_status_t status)
    const noexcept -> std::optional<iop::StaticString> {
  std::optional<iop::StaticString> ret;
  IOP_TRACE();
  switch (status) {
  case STATION_IDLE:
    ret.emplace(F("STATION_IDLE"));
    break;
  case STATION_CONNECTING:
    ret.emplace(F("STATION_CONNECTING"));
    break;
  case STATION_WRONG_PASSWORD:
    ret.emplace(F("STATION_WRONG_PASSWORD"));
    break;
  case STATION_NO_AP_FOUND:
    ret.emplace(F("STATION_NO_AP_FOUND"));
    break;
  case STATION_CONNECT_FAIL:
    ret.emplace(F("STATION_CONNECT_FAIL"));
    break;
  case STATION_GOT_IP:
    ret.emplace(F("STATION_GOT_IP"));
    break;
  }
  if (!ret.has_value())
    this->logger.error(iop::StaticString(F("Unknown status: ")).toStdString() + std::to_string(status));
  return ret;
}

auto CredentialsServer::connect(iop::StringView ssid,
                                iop::StringView password) const noexcept
    -> void {
  IOP_TRACE();
  this->logger.info(F("Connect: "), ssid);
  if (wifi_station_get_connect_status() == STATION_CONNECTING) {
    const iop::InterruptLock _guard;
    wifi_station_disconnect();
  }

  WiFi.begin(ssid.get(), std::move(password).get());

  if (WiFi.waitForConnectResult() == -1) {
    this->logger.error(F("Wifi authentication timed out"));
    return;
  }

  if (!iop::Network::isConnected()) {
    const auto status = wifi_station_get_connect_status();
    auto maybeStatusStr = this->statusToString(status);
    if (!maybeStatusStr.has_value())
      return; // It already will be logged by statusToString;

    const auto statusStr = iop::unwrap(maybeStatusStr, IOP_CTX());
    this->logger.error(F("Invalid wifi credentials ("), statusStr, F("): "),
                       std::move(ssid));
  }
}

auto CredentialsServer::authenticate(iop::StringView username,
                                     iop::StringView password,
                                     const Api &api) const noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  WiFi.mode(WIFI_STA);

  auto authToken = api.authenticate(username, std::move(password));
  
  WiFi.mode(WIFI_AP_STA);
  this->logger.info(F("Tried to authenticate"));
  if (IS_ERR(authToken)) {
    const auto &status = UNWRAP_ERR_REF(authToken);

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
  return std::make_optional(UNWRAP_OK(authToken));
}

auto CredentialsServer::serve(const std::optional<WifiCredentials> &storedWifi,
                              const Api &api) noexcept
    -> std::optional<AuthToken> {
  IOP_TRACE();
  this->start();

  const auto now = millis();

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
    this->connect(stored.ssid.asString(), stored.password.asString());

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
  dnsServer.processNextRequest();
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
