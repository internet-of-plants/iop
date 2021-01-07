#include "server.hpp"

#ifndef SERVER_DISABLED
#include "api.hpp"
#include "configuration.h"
#include "utils.hpp"

#include "IPAddress.h"
#include "WiFiClient.h"
#include "bits/basic_string.h"
#include "string.h"
#include <string>

// TODO: make sure this captive portal can't be bypassed

const uint64_t intervalTryFlashWifiCredentialsMillis = 600000; // 10 minutes

const uint64_t intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes

const uint64_t intervalTryHardcodedIopCredentialsMillis = 3600000; // 1 hour

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
    wifiHTML,
    "<h3><center>Please provide your Wifi credentials, so we can connect to "
    "it</center></h3>\r\n"
    "<div><div><strong>Network name:</strong></div><input name='ssid' "
    "type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='password' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(
    iopHTML,
    "<h3><center>Please provide your Iop credentials, so we can get an "
    "authentication token to use</center></h3>\r\n"
    "<div><div><strong>Email:</strong></div><input name='iopEmail' type='text' "
    "style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='iopPassword' "
    "type='password' style='width:100%' /></div>\r\n");

PROGMEM_STRING(pageHTMLEnd, "<br>\r\n"
                            "<input type='submit' value='Submit' />\r\n"
                            "</form></body></html>");

#include <unordered_map>

Option<std::pair<std::string, std::string>> credentialsWifi;
Option<std::pair<std::string, std::string>> credentialsIop;

void CredentialsServer::makeRouter(const Server &s) const noexcept {
  const auto api = this->api;
  const auto loggerLevel = this->logger.level();
  const auto flash = this->flash;

  s->on(F("/submit"), [s, loggerLevel]() {
    const Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
    logger.debug(F("Received form with credentials"));

    if (s->hasArg(F("ssid")) && s->hasArg(F("password"))) {
      const auto ssid = s->arg(F("ssid")).c_str();
      const auto psk = s->arg(F("password")).c_str();
      credentialsWifi = std::pair<std::string, std::string>(ssid, psk);
    }

    if (s->hasArg(F("iopEmail")) && s->hasArg(F("iopPassword"))) {
      const auto email = s->arg(F("iopEmail")).c_str();
      const auto password = s->arg(F("iopPassword")).c_str();
      credentialsIop = std::pair<std::string, std::string>(email, password);
    }

    s->sendHeader(F("Location"), F("/"));
    s->send_P(302, PSTR("text/plain"), PSTR(""));
  });

  s->onNotFound([s, api, flash, loggerLevel]() {
    const Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
    logger.debug(F("Serving captive portal HTML"));

    const auto isConnected = !api->isConnected();
    const auto needsIopAuth = flash->readAuthToken().isNone();

    auto len = pageHTMLStart.length() + pageHTMLEnd.length();
    if (isConnected)
      len += wifiHTML.length();
    if (needsIopAuth)
      len += iopHTML.length();
    s->setContentLength(len);

    s->send_P(200, PSTR("text/html"), pageHTMLStart.asCharPtr());
    if (isConnected)
      s->sendContent_P(wifiHTML.asCharPtr());
    if (needsIopAuth)
      s->sendContent_P(iopHTML.asCharPtr());
    s->sendContent_P(pageHTMLEnd.asCharPtr());
  });
}

void CredentialsServer::start() noexcept {
  if (server.isNone()) {
    this->logger.info(F("Setting our own wifi access point"));

    WiFi.mode(WIFI_AP_STA);

    // TODO: the password should be random (passed at compile time)
    // But also accessible externally (like a sticker in the hardware).
    // So not dynamic
    const auto staticIp = IPAddress(192, 168, 1, 1);
    WiFi.softAPConfig(staticIp, staticIp, IPAddress(255, 255, 255, 0));

    const auto hash = StringView(this->api->macAddress()).hash();
    // TODO: Should we use String here because of the F() optimization? Or is
    // String still worse at optimizations when comparing to std::string?
    const auto ssid = std::string(PSTR("iop-")) + std::to_string(hash);

    WiFi.setAutoReconnect(false);
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();

    WiFi.softAP(ssid.c_str(), PSTR("le$memester#passwordz"), 2);
    WiFi.setAutoReconnect(true);
    WiFi.begin();

    // Makes it a captive portal (redirects all wifi trafic to ir)
    auto dns = make_unique<DNSServer>();
    dns->setErrorReplyCode(DNSReplyCode::NoError);
    dns->start(53, F("*"), WiFi.softAPIP());

    auto s = std::make_shared<ESP8266WebServer>(80);
    this->makeRouter(s);
    s->begin();

    const auto ip = WiFi.softAPIP().toString();
    this->logger.info(F("Opened captive portal: "), ip);

    // We can move them here because they won't ever be used
    // Beware of adding things to this method, this should always be at the end
    server = std::move(s);
    dnsServer = std::move(dns);
  }
}

void CredentialsServer::close() noexcept {
  if (this->server.isSome()) {
    UNWRAP(this->server)->close();
    WiFi.mode(WIFI_STA);
  }
  if (this->dnsServer.isSome()) {
    UNWRAP(this->dnsServer)->stop();
  }
}

Option<StaticString> CredentialsServer::statusToString(
    const station_status_t status) const noexcept {
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

station_status_t
CredentialsServer::connect(const StringView ssid,
                           const StringView password) const noexcept {
  if (wifi_station_get_connect_status() == STATION_CONNECTING) {
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
  }

  WiFi.begin(ssid.get(), password.get(), 3);

  if (WiFi.waitForConnectResult() == -1) {
    this->logger.warn(F("Wifi authentication timed out"));
    return wifi_station_get_connect_status();

  } else {
    const auto status = wifi_station_get_connect_status();
    if (!this->api->isConnected()) {
      const auto maybeStatusStr = CredentialsServer::statusToString(status);
      if (maybeStatusStr.isSome()) {
        const auto statusStr = UNWRAP_REF(maybeStatusStr);
        this->logger.warn(F("Invalid wifi credentials ("), statusStr, F("): "),
                          ssid, F(", "), password);
      }
    }
    return status;
  }
}

/// Forbidden (403): invalid credentials
Result<AuthToken, ApiStatus>
CredentialsServer::authenticate(const StringView username,
                                const StringView password) const noexcept {
  auto authToken = this->api->authenticate(username, password);
  if (IS_ERR(authToken)) {
    const auto &status = UNWRAP_ERR_REF(authToken);

    switch (status) {
    case ApiStatus::FORBIDDEN:
      this->logger.warn(F("Invalid IoP credentials ("),
                        this->api->network().apiStatusToString(status),
                        F("): "), username, F(", "), password);
      return status;

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      panic_(F("CredentialsServer::authenticate internal buffer overflow"));

    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      this->logger.warn(F("CredentialsServer::authenticate network error"));
      return status;

    case ApiStatus::BROKEN_SERVER:
    case ApiStatus::NOT_FOUND:
      this->logger.warn(F("CredentialsServer::authenticate server error"));
      return status;

    case ApiStatus::OK: // Cool beans
      return status;

    case ApiStatus::MUST_UPGRADE:
      interruptEvent = InterruptEvent::MUST_UPGRADE;
      return status;
    }

    const auto str = this->api->network().apiStatusToString(status);
    this->logger.error(F("CredentialsServer::authenticate bad status "), str);
    return status;
  } else {
    return UNWRAP_OK(authToken);
  }
}

Result<Option<AuthToken>, ServeError>
CredentialsServer::serve(const Option<WifiCredentials> &storedWifi,
                         const Option<AuthToken> &authToken) noexcept {
  this->start();

  if (credentialsWifi.isSome()) {
    const auto cred = UNWRAP(credentialsWifi);
    this->connect(cred.first, cred.second);
  }

  if (credentialsIop.isSome()) {
    const auto cred = UNWRAP(credentialsIop);
    auto result = this->authenticate(cred.first, cred.second);
    if (IS_OK(result)) {
      const auto token = UNWRAP_OK(result);
      return Option<AuthToken>(token);
    }
  }

  const auto now = millis();
  if (!this->api->isConnected() && storedWifi.isSome() &&
      this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials =
        now + intervalTryFlashWifiCredentialsMillis;

    const auto &stored = UNWRAP_REF(storedWifi);
    const auto status =
        this->connect(stored.ssid.asString(), stored.password.asString());
    if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = 0;
      return ServeError::INVALID_WIFI_CONFIG;
    }
  }

  if (!this->api->isConnected() && wifiNetworkName.isSome() &&
      wifiPassword.isSome() && this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials =
        now + intervalTryHardcodedWifiCredentialsMillis;
    const auto &ssid = UNWRAP_REF(wifiNetworkName);
    const auto &password = UNWRAP_REF(wifiPassword);
    const auto status = this->connect(ssid, password);
    if (status == STATION_GOT_IP) {
      this->nextTryHardcodedWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = now + 24 * 3600 * 1000;
    }
  }

  if (this->api->isConnected() && authToken.isNone() &&
      this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials =
        now + intervalTryHardcodedIopCredentialsMillis;
    if (iopEmail.isSome() && iopPassword.isSome()) {
      const auto &email = UNWRAP_REF(iopEmail);
      const auto &password = UNWRAP_REF(iopPassword);
      auto res = this->authenticate(email, password);

      if (IS_OK(res)) {
        this->nextTryHardcodedIopCredentials = 0;
        return OK(res);
      } else if (UNWRAP_ERR(res) == ApiStatus::FORBIDDEN) {
        this->nextTryHardcodedIopCredentials = now + 24 + 3600 + 1000;
      }
    }
  }

  UNWRAP_MUT(this->dnsServer)->processNextRequest();
  UNWRAP_MUT(this->server)->handleClient();
  return Option<AuthToken>();
}
#endif

#ifdef IOP_SERVER_DISABLED
Result<Option<AuthToken>, ServeError>
CredentialsServer::serve(const Option<WifiCredentials> &storedWifi,
                         const Option<AuthToken> &authToken) noexcept {
  (void)storedWifi;
  return Result<Option<AuthToken>, ServeError>(authToken.asRef().map<AuthToken>(
      [](const std::reference_wrapper<const AuthToken> token) {
        return token.get();
      }));
}
void CredentialsServer::close() noexcept {}
void CredentialsServer::start() noexcept {}
#endif
