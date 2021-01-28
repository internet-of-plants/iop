#include "server.hpp"

#include "IPAddress.h"
#include "WiFiClient.h"

#ifndef IOP_SERVER_DISABLED
#include "api.hpp"
#include "configuration.h"
#include "utils.hpp"

#include "bits/basic_string.h"
#include <cstring>
#include <string>

#include "static_string.hpp"
#include "string_view.hpp"

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

// TODO: add js to disable fields according to wifi and iop checkboxes?
PROGMEM_STRING(
    wifiOverwriteHTML,
    "<h3><center>It seems you already have your wifi credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center></h3>\r\n"
    "<div><input type='checkbox' name='wifi'><label "
    "for='wifi'>Overwrite wifi credentials</label></div>"
    "<div><div><strong>Network name:</strong></div><input name='ssid' "
    "type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='password' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(
    wifiHTML,
    "<h3><center>Please provide your Wifi credentials, so we can connect to "
    "it.</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='wifi'></div>"
    "<div><div><strong>Network name:</strong></div><input name='ssid' "
    "type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='password' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(
    iopOverwriteHTML,
    "<h3><center>It seems you already have your Iop credentials set, if you "
    "want to rewrite it, please set the checkbox below and fill the "
    "fields. Otherwise they will be ignored</center></h3>\r\n"
    "<div><input type='checkbox' name='iop'><label "
    "for='iop'>Overwrite Iop credentials</label></div>"
    "<div><div><strong>Email:</strong></div><input name='iopEmail' type='text' "
    "style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='iopPassword' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(
    iopHTML,
    "<h3><center>Please provide your Iop credentials, so we can get an "
    "authentication token to use</center></h3>\r\n"
    "<div><input type='hidden' value='true' name='iop'></div>"
    "<div><div><strong>Email:</strong></div><input name='iopEmail' type='text' "
    "style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='iopPassword' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(pageHTMLEnd, "<br>\r\n"
                            "<input type='submit' value='Submit' />\r\n"
                            "</form></body></html>");

#include <unordered_map>

// We use this globals to share messages from the callbacks
static Option<std::pair<String, String>> credentialsWifi;
static Option<std::pair<String, String>> credentialsIop;

constexpr const uint8_t serverPort = 80;
static ESP8266WebServer server(serverPort); // NOLINT cert-err58-cpp

constexpr const uint8_t dnsPort = 53;
static DNSServer dnsServer; // NOLINT cert-err58-cpp

void CredentialsServer::setup() const noexcept {
  IOP_TRACE();
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  const auto loggerLevel = this->logger.level();

  // Self reference, but it's to a static
  auto *s = &server;
  server.on(F("/submit"), [s, loggerLevel]() {
    IOP_TRACE();
    const static Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
    logger.debug(F("Received form with credentials"));

    if (s->hasArg(F("wifi")) && s->hasArg(F("ssid")) &&
        s->hasArg(F("password"))) {
      const auto ssid = s->arg(F("ssid"));
      const auto psk = s->arg(F("password"));
      if (!ssid.isEmpty() && !psk.isEmpty())
        credentialsWifi = std::pair<String, String>(ssid, psk);
    }

    if (s->hasArg(F("iop")) && s->hasArg(F("iopEmail")) &&
        s->hasArg(F("iopPassword"))) {
      const auto email = s->arg(F("iopEmail"));
      const auto password = s->arg(F("iopPassword"));
      if (!email.isEmpty() && !password.isEmpty())
        credentialsIop = std::pair<String, String>(email, password);
    }

    s->sendHeader(F("Location"), F("/"));
    s->send_P(HTTP_CODE_FOUND, PSTR("text/plain"), PSTR(""));
  });

  server.onNotFound([s, loggerLevel]() {
    IOP_TRACE();
    const static Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
    const static Flash flash(loggerLevel);
    logger.debug(F("Serving captive portal HTML"));

    const auto mustConnect = !Network::isConnected();
    const auto needsIopAuth = flash.readAuthToken().isNone();

    auto len = pageHTMLStart.length() + pageHTMLEnd.length();

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

    const static auto hash = utils::macAddress().asString().hash();
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

auto CredentialsServer::statusToString(
    const station_status_t status) const noexcept -> Option<StaticString> {
  IOP_TRACE();
  switch (status) {
  case STATION_IDLE:
    return StaticString(F("STATION_IDLE"));
  case STATION_CONNECTING:
    return StaticString(F("STATION_CONNECTING"));
  case STATION_WRONG_PASSWORD:
    return StaticString(F("STATION_WRONG_PASSWORD"));
  case STATION_NO_AP_FOUND:
    return StaticString(F("STATION_NO_AP_FOUND"));
  case STATION_CONNECT_FAIL:
    return StaticString(F("STATION_CONNECT_FAIL"));
  case STATION_GOT_IP:
    return StaticString(F("STATION_GOT_IP"));
  }
  this->logger.error(String(F("Unknown status: ")) + String(status));
  return Option<StaticString>();
}

auto CredentialsServer::connect(const StringView ssid,
                                const StringView password) const noexcept
    -> void {
  IOP_TRACE();
  if (wifi_station_get_connect_status() == STATION_CONNECTING) {
    ETS_UART_INTR_DISABLE(); // NOLINT hicpp-signed-bitwise
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE(); // NOLINT hicpp-signed-bitwise
  }

  // TODO: should we use WiFi.setPersistent(false) and save it to flash on our
  // own when connection succeeds? It seems invalid credentials here still get
  // stored to flash even if they fail
  WiFi.begin(ssid.get(), password.get());

  if (WiFi.waitForConnectResult() == -1) {
    this->logger.error(F("Wifi authentication timed out"));
    return;
  }

  if (!Network::isConnected()) {
    const auto status = wifi_station_get_connect_status();
    const auto maybeStatusStr = this->statusToString(status);
    if (maybeStatusStr.isNone())
      return; // It already will be logged by statusToString;

    const auto statusStr = UNWRAP_REF(maybeStatusStr);
    this->logger.error(F("Invalid wifi credentials ("), statusStr, F("): "),
                       ssid);
  }
}

/// Forbidden (403): invalid credentials
auto CredentialsServer::authenticate(const StringView username,
                                     const StringView password,
                                     const MacAddress &mac,
                                     const Api &api) const noexcept
    -> Option<AuthToken> {
  IOP_TRACE();
  WiFi.mode(WIFI_STA);
  auto authToken = api.authenticate(username, password, mac);
  WiFi.mode(WIFI_AP_STA);
  this->logger.info(F("Tried to authenticate"));

  if (IS_ERR(authToken)) {
    const auto &status = UNWRAP_ERR_REF(authToken);

    switch (status) {
    case ApiStatus::FORBIDDEN:
      this->logger.error(F("Invalid IoP credentials ("),
                         Network::apiStatusToString(status), F("): "),
                         username);
      return Option<AuthToken>();

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      panic_(F("CredentialsServer::authenticate internal buffer overflow"));

    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      this->logger.error(F("CredentialsServer::authenticate network error"));
      return Option<AuthToken>();

    case ApiStatus::BROKEN_SERVER:
    case ApiStatus::NOT_FOUND:
      this->logger.error(F("CredentialsServer::authenticate server error"));
      return Option<AuthToken>();

    case ApiStatus::OK:
    case ApiStatus::MUST_UPGRADE:
      // On success those shouldn't be triggered
      panic_(F("Unreachable"));
    }

    const auto str = Network::apiStatusToString(status);
    this->logger.crit(F("CredentialsServer::authenticate bad status: "), str);
    return Option<AuthToken>();
  }
  return UNWRAP_OK(authToken);
}

auto CredentialsServer::serve(const Option<WifiCredentials> &storedWifi,
                              const MacAddress &mac, const Api &api) noexcept
    -> Option<AuthToken> {
  IOP_TRACE();
  this->start();

  const auto now = millis();

  // The user provided those informations through the web form
  // But we shouldn't act on it inside the server's callback, as callback
  // should be rather simple, so we use globals. UNWRAP moves them out on use.

  if (credentialsWifi.isSome()) {
    const auto cred = UNWRAP(credentialsWifi);
    this->connect(cred.first, cred.second);

  } else if (Network::isConnected() && credentialsIop.isSome()) {
    const auto cred = UNWRAP(credentialsIop);
    auto tok = this->authenticate(cred.first, cred.second, mac, api);
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
  } else if (!Network::isConnected() && storedWifi.isSome() &&
             this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials =
        now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = UNWRAP_REF(storedWifi);
    this->logger.info(
        F("Trying to connect to wifi with flash stored credentials"));
    this->connect(stored.ssid.asString(), stored.password.asString());

    // WiFi Credentials hardcoded at "configuration.h"
    //
    // Ideally it won't be wrong, but that's the price of hardcoding, if it's
    // not updated it may just be. Since it can't be deleted we must retry,
    // but with a bigish interval.
  } else if (!Network::isConnected() && wifiNetworkName.isSome() &&
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
  return Option<AuthToken>();
}
#else
auto CredentialsServer::serve(const Option<WifiCredentials> &storedWifi,
                              const MacAddress &mac, const Api &api) noexcept
    -> Option<AuthToken> {
  IOP_TRACE();
  (void)mac;
  (void)storedWifi;
  return Option<AuthToken>();
}
void CredentialsServer::setup() const noexcept { IOP_TRACE(); }
void CredentialsServer::close() noexcept { IOP_TRACE(); }
void CredentialsServer::start() noexcept { IOP_TRACE(); }
#endif
