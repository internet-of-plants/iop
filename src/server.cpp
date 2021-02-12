#include "server.hpp"

#ifndef IOP_SERVER_DISABLED
#include "configuration.hpp"
#include "core/interrupt.hpp"
#include "flash.hpp"

constexpr const uint64_t intervalTryFlashWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

constexpr const uint64_t intervalTryHardcodedWifiCredentialsMillis =
    60 * 60 * 1000; // 1 hour

PROGMEM_STRING(pageHTMLStart,
               "<!DOCTYPE HTML>\r\n"
               "<html><body>\r\n"
               "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
               "  <h4><center>If, in the future, you want to reset the "
               "configurations set here, just press the factory reset button "
               "for at least 15 seconds</center></h4>"
               "<form style='margin: 0 auto; width: 500px;' action='/submit' "
               "method='POST'>\r\n");

PROGMEM_STRING(
    wifiOverwriteHTML,
    "<h3>"
    "  <center>It seems you already have your wifi credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center>"
    "</h3>\r\n"
    "<div>"
    "  <input type='checkbox' name='wifi'>"
    "  <label for='wifi'>Overwrite wifi credentials</label>"
    "</div>"
    "<div class=\"wifi\" style=\"display: 'none'\">"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div class=\"wifi\" style=\"display: 'none'\">"
    "  <div><strong>Password:</strong></div>"
    "  <input name='password' type='password' style='width:100%' />"
    "</div>\r\n");

PROGMEM_STRING(
    wifiHTML,
    "<h3><center>"
    "Please provide your Wifi credentials, so we can connect to it."
    "</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='wifi'></div>"
    "<div>"
    "  <div><strong>Network name:</strong></div>"
    "  <input name='ssid' type='text' style='width:100%' />"
    "</div>\r\n"
    "<div><div><strong>Password:</strong></div>"
    "<input name='password' type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(
    iopOverwriteHTML,
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
    "</div>\r\n");

PROGMEM_STRING(
    iopHTML, "<h3><center>"
             "  Please provide your Iop credentials, so we can get an "
             "authentication token to use"
             "</center></h3>\r\n"
             "<div><input type='hidden' value='true' name='iop'></div>"
             "<div>"
             "  <div><strong>Email:</strong></div>"
             "  <input name='iopEmail' type='text' style='width:100%' />"
             "</div>\r\n"
             "<div>"
             "  <div><strong>Password:</strong></div>"
             "  <input name='iopPassword' type='password' style='width:100%' />"
             "</div>\r\n");

PROGMEM_STRING(script,
               "<script type=\"javascript\">"
               "const wifi = document.querySelector(\"input[name=\"wifi\"]\");"
               "wifi.addEventListener(\"onchange\", ev => {"
               "  ev.getElementsByClassName(\"wifi\").forEach(el => {"
               "    if (ev.currentTarget.checked) {"
               "      el.style.display = \"block\";"
               "    } else {"
               "      el.style.display = \"none\";"
               "    }"
               "  });"
               "});"
               "const iop = document.querySelector(\"input[name=\"iop\"]\"));"
               "iop.addEventListener(\"onchange\", ev => {"
               "  ev.getElementsByClassName(\"iop\").forEach(el => {"
               "    if (ev.currentTarget.checked) {"
               "      el.style.display = \"block\";"
               "    } else {"
               "      el.style.display = \"none\";"
               "    }"
               "  });"
               "});"
               "</script>");

PROGMEM_STRING(pageHTMLEnd, "<br>\r\n"
                            "<input type='submit' value='Submit' />\r\n"
                            "</form></body></html>");

// We use this globals to share messages from the callbacks
static iop::Option<std::pair<String, String>> credentialsWifi;
static iop::Option<std::pair<String, String>> credentialsIop;

constexpr const uint8_t serverPort = 80;
static ESP8266WebServer server(serverPort);

constexpr const uint8_t dnsPort = 53;
static DNSServer dnsServer;

void CredentialsServer::setup() const noexcept {
  IOP_TRACE();
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  const auto loggerLevel = this->logger.level();

  // Self reference, but it's to a static
  auto *s = &server;
  server.on(F("/submit"), [s, loggerLevel]() {
    IOP_TRACE();
    if (loggerLevel >= iop::LogLevel::DEBUG)
      iop::Log::print(
          F("[DEBUG] [RAW] SERVER_CALLBACK: Received credentials form\n"),
          iop::LogLevel::DEBUG, iop::LogType::STARTEND);

    if (s->hasArg(F("wifi")) && s->hasArg(F("ssid")) &&
        s->hasArg(F("password"))) {
      const auto ssid = s->arg(F("ssid"));
      const auto psk = s->arg(F("password"));
      if (!ssid.isEmpty() && !psk.isEmpty())
        credentialsWifi.emplace(ssid, psk);
    }

    if (s->hasArg(F("iop")) && s->hasArg(F("iopEmail")) &&
        s->hasArg(F("iopPassword"))) {
      const auto email = s->arg(F("iopEmail"));
      const auto password = s->arg(F("iopPassword"));
      if (!email.isEmpty() && !password.isEmpty())
        credentialsIop.emplace(email, password);
    }

    s->sendHeader(F("Location"), F("/"));
    s->send_P(HTTP_CODE_FOUND, PSTR("text/plain"), PSTR(""));
  });

  server.onNotFound([s, loggerLevel]() {
    IOP_TRACE();
    const static Flash flash(loggerLevel);
    if (loggerLevel >= iop::LogLevel::DEBUG)
      iop::Log::print(
          F("[DEBUG] [RAW] SERVER_CALLBACK: Serving captive portal HTML\n"),
          iop::LogLevel::DEBUG, iop::LogType::STARTEND);

    const auto mustConnect = !iop::Network::isConnected();
    const auto needsIopAuth = flash.readAuthToken().isNone();

    auto len = pageHTMLStart.length() + pageHTMLEnd.length() + script.length();

    if (mustConnect) {
      len += wifiHTML.length();
    } else {
      len += wifiOverwriteHTML.length();
    }

    if (needsIopAuth) {
      len += iopHTML.length();
    } else {
      len += iopOverwriteHTML.length();
    }

    s->setContentLength(len);

    s->send_P(HTTP_CODE_OK, PSTR("text/html"), pageHTMLStart.asCharPtr());

    if (mustConnect) {
      s->sendContent_P(wifiHTML.asCharPtr());
    } else {
      s->sendContent_P(wifiOverwriteHTML.asCharPtr());
    }

    if (needsIopAuth) {
      s->sendContent_P(iopHTML.asCharPtr());
    } else {
      s->sendContent_P(iopOverwriteHTML.asCharPtr());
    }

    s->sendContent_P(script.asCharPtr());
    s->sendContent_P(pageHTMLEnd.asCharPtr());
  });
}

void CredentialsServer::start() noexcept {
  IOP_TRACE();
  if (!this->isServerOpen) {
    this->isServerOpen = true;
    this->logger.info(F("Setting our own wifi access point"));

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
    server.begin();

    // Makes it a captive portal (redirects all wifi trafic to it)
    dnsServer.start(dnsPort, F("*"), WiFi.softAPIP());

    const auto ip = WiFi.softAPIP().toString();
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
    const noexcept -> iop::Option<iop::StaticString> {
  iop::Option<iop::StaticString> ret;
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
  if (ret.isNone())
    this->logger.error(String(F("Unknown status: ")) + String(status));
  return std::move(ret);
}

auto CredentialsServer::connect(iop::StringView ssid,
                                iop::StringView password) const noexcept
    -> void {
  IOP_TRACE();
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
    const auto maybeStatusStr = this->statusToString(status);
    if (maybeStatusStr.isNone())
      return; // It already will be logged by statusToString;

    const auto statusStr = UNWRAP_REF(maybeStatusStr);
    this->logger.error(F("Invalid wifi credentials ("), statusStr, F("): "),
                       std::move(ssid));
  }
}

auto CredentialsServer::authenticate(iop::StringView username,
                                     iop::StringView password,
                                     const Api &api) const noexcept
    -> iop::Option<AuthToken> {
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
      return iop::Option<AuthToken>();

    case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
      iop_panic(F("CredentialsServer::authenticate internal buffer overflow"));

    // Already logged at the Network level
    case iop::NetworkStatus::CONNECTION_ISSUES:
    case iop::NetworkStatus::BROKEN_SERVER:
      // Nothing to be done besides retrying later
      return iop::Option<AuthToken>();

    case iop::NetworkStatus::OK:
      // On success an AuthToken is returned, not OK
      iop_panic(F("Unreachable"));
    }

    const auto str = iop::Network::apiStatusToString(status);
    this->logger.crit(F("CredentialsServer::authenticate bad status: "), str);
    return iop::Option<AuthToken>();
  }
  return iop::some(UNWRAP_OK(authToken));
}

auto CredentialsServer::serve(const iop::Option<WifiCredentials> &storedWifi,
                              const Api &api) noexcept
    -> iop::Option<AuthToken> {
  IOP_TRACE();
  this->start();

  const auto now = millis();

  // The user provided those informations through the web form
  // But we shouldn't act on it inside the server's callback, as callback
  // should be rather simple, so we use globals. UNWRAP moves them out on use.

  if (credentialsWifi.isSome()) {
    const auto cred = UNWRAP(credentialsWifi);
    this->connect(cred.first, cred.second);

  } else if (iop::Network::isConnected() && credentialsIop.isSome()) {
    const auto cred = UNWRAP(credentialsIop);
    auto tok = this->authenticate(cred.first, cred.second, api);
    if (tok.isSome())
      return tok;

    // WiFi Credentials stored in flash

    // Wifi connection error is hard to debug
    // (see countless open issues about it at esp8266/Arduino's github)
    // One example citing others:
    // https://github.com/esp8266/Arduino/issues/7432
    //
    // So we never delete a flash stored wifi credentials (outside of factory
    // reset), we have a timer to avoid constantly retrying a bad credential.
  } else if (!iop::Network::isConnected() && storedWifi.isSome() &&
             this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials =
        now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = UNWRAP_REF(storedWifi);
    this->logger.info(
        F("Trying to connect to wifi with flash stored credentials"));
    this->connect(stored.ssid.asString(), stored.password.asString());

    // WiFi Credentials hardcoded at "configuration.hpp"
    //
    // Ideally it won't be wrong, but that's the price of hardcoding, if it's
    // not updated it may just be. Since it can't be deleted we must retry,
    // but with a bigish interval.
  } else if (!iop::Network::isConnected() && wifiNetworkName.isSome() &&
             wifiPassword.isSome() &&
             this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials =
        now + intervalTryHardcodedWifiCredentialsMillis;
    const auto &ssid = UNWRAP_REF(wifiNetworkName);
    const auto &psk = UNWRAP_REF(wifiPassword);
    this->logger.info(
        F("Trying to connect to wifi with hardcoded credentials"));
    this->connect(String(ssid.get()), String(psk.get()));
  }

  // Give processing time to the servers
  dnsServer.processNextRequest();
  server.handleClient();
  return iop::Option<AuthToken>();
}
#else
auto CredentialsServer::serve(const iop::Option<WifiCredentials> &storedWifi,
                              const MacAddress &mac, const Api &api) noexcept
    -> iop::Option<AuthToken> {
  IOP_TRACE();
  (void)*this;
  (void)api;
  (void)mac;
  (void)storedWifi;
  return iop::Option<AuthToken>();
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
