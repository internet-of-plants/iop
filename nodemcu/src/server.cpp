#include <server.hpp>

#ifndef IOP_SERVER_DISABLED
#include <configuration.h>
#include <utils.hpp>

#include <bits/basic_string.h>
#include <IPAddress.h>
#include <WiFiClient.h>

const unsigned long intervalTryFlashWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedIopCredentialsMillis = 3600000; // 1 hour

// TODO: add csrf protection

const char pageHTMLStart[] PROGMEM =
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h4><center>If, in the future, you want to reset the configuration set here just press the factory reset button for at least 10 seconds</center></h4>"
    "<form style='margin: 0 auto; width: 500px;' action='/submit' method='POST'>\r\n";

const char wifiHTML[] PROGMEM =
    "<h3><center>Please provide your Wifi credentials, so we can connect to it</center></h3>\r\n"
    "<div><div><strong>Network name:</strong></div><input name='ssid' type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='password' type='password' style='width:100%' /></div>\r\n";

const char iopHTML[] PROGMEM =
    "<h3><center>Please provide your Iop credentials, so we can get an authentication token to use</center></h3>\r\n"
    "<div><div><strong>Email:</strong></div><input name='iopEmail' type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='iopPassword' type='password' style='width:100%' /></div>\r\n";

const char pageHTMLEnd[] PROGMEM =
    "<br>\r\n"
    "<input type='submit' value='Submit' />\r\n"
    "</form></body></html>";

#include <unordered_map>

static station_status_t authenticateWifi(const Log & logger, const Api & api, const String & ssid, const String & password) {
  if (wifi_station_get_connect_status() == STATION_CONNECTING) {
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
  }

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == -1) {
    logger.warn(STATIC_STRING("Wifi authentication timed out"), START, STATIC_STRING("\n"), false);
    return STATION_CONNECT_FAIL;
  }

  const auto status = wifi_station_get_connect_status();
  if (!api.isConnected()) {
    logger.warn(STATIC_STRING("Invalid credentials ("), START, STATIC_STRING(" "), false);
    logger.warn(String(status), CONTINUITY, STATIC_STRING("): "), false);
    logger.warn(ssid, CONTINUITY, STATIC_STRING(", "), false);
    logger.warn(password, CONTINUITY, STATIC_STRING("\n"), false);
  }
  return status;
}

Option<AuthToken> authenticateIop(const Log & logger, const Api &api, const String username, const String password) {
  auto authToken = api.authenticate(username, password);
  if (authToken.isSome()) {
    const AuthToken token = authToken.expect(STATIC_STRING("Inside authenticateIop: Auth Token is none but shouldn't."));
    return Option<AuthToken>(token);
  }

  logger.warn(STATIC_STRING("Invalid wifi credentials:"), START, STATIC_STRING(" "), false);
  logger.warn(username, CONTINUITY, STATIC_STRING(", "), false);
  logger.warn(password, CONTINUITY, STATIC_STRING("\n"), false);
  return Option<AuthToken>();
}

void CredentialsServer::start() {
  if (server.isNone()) {
    this->logger.info(STATIC_STRING("Setting our own wifi access point"));

    // TODO: the password should be random (passed at compile time)
    // But also accessible externally (like a sticker in the hardware).
    WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    const auto hash = std::to_string(hashString(this->api->macAddress()));
    const auto ssid = String("iop-") + String(hash.c_str());
    WiFi.softAP(ssid, "le$memester#passwordz");

    // Makes it a captive portal (redirects all wifi trafic to us)
    auto dns = std::unique_ptr<DNSServer>(new DNSServer());
    dns->setErrorReplyCode(DNSReplyCode::NoError);
    dns->start(53, F("*"), WiFi.softAPIP());

    const auto api = this->api;
    const auto loggerLevel = this->logger.level();
    const auto flash = this->flash;

    auto s = std::make_shared<ESP8266WebServer>(80);
    s->on(F("/submit"), [s, loggerLevel, api]() {
      const Log logger(loggerLevel, STATIC_STRING("SERVER_CALLBACK"));
      logger.info(STATIC_STRING("Received form with credentials"));
      s->sendHeader(F("Connection"), F("close"));
      if (s->hasArg(F("ssid")) && s->hasArg(F("password"))) {
      const station_status_t status = authenticateWifi(logger, *api, s->arg(F("ssid")), s->arg(F("password")));
        if (status == STATION_GOT_IP) {
          s->send(200, F("text/plain"), F("Authentication succeeded :)"));
        } else if (status == STATION_WRONG_PASSWORD) {
          s->send(403, F("text/plain"), F("Wrong password"));
        } else {
          s->send(404, F("text/plain"), String(F("Authentication failed :(. Status: ")) + String(status));
        }
      }

      if (s->hasArg(F("iopEmail")) && s->hasArg(F("iopPassword"))) {
        authenticateIop(logger, *api, s->arg(F("iopEmail")), s->arg(F("iopPassword")));
      }
    });
    s->onNotFound([s, api, loggerLevel, flash]() {
      const Log logger(loggerLevel, STATIC_STRING("SERVER_CALLBACK"));
      logger.info(STATIC_STRING("Serving captive portal HTML"), START, STATIC_STRING("\n"), false);

      const auto wifi = !api->isConnected();
      const auto iop = flash->readAuthToken().isNone();

      s->sendHeader(F("Connection"), F("close"));

      auto len = strlen_P(pageHTMLStart) + strlen_P(pageHTMLEnd);
      if (wifi) len += strlen_P(wifiHTML);
      if (iop) len += strlen_P(iopHTML);
      s->setContentLength(len);

      s->send_P(200, PSTR("text/html"), pageHTMLStart);
      if (wifi) s->sendContent_P(wifiHTML);
      if (iop) s->sendContent_P(iopHTML);
      s->sendContent_P(pageHTMLEnd);
    });
    s->begin();
    this->logger.info(STATIC_STRING("Opened captive portal"));
    this->logger.info(WiFi.softAPIP().toString());
    server = std::move(s);
    dnsServer = std::move(dns);
  }
}

void CredentialsServer::close() {
  if (this->server.isSome()) {
    this->server.take().expect(STATIC_STRING("Inside CredentialsServer::close, server is None but shouldn't be"))->close();
  }
  if (this->dnsServer.isSome()) {
    this->dnsServer.take().expect(STATIC_STRING("Inside CredentialsServer::close, dnsServer is None but shouldn't be"))->stop();
  }
}

/// Abstracts away wifi and iop credentials acquisition.
/// If there are hardcoded credentials this will use them (and avoid a floods if they are invalid)
/// If there are credentials stored in flash memory it will use them, clearing them when proved invalid
///
/// If nothing else works it will serve a HTTP server at port 80 that provides a HTML form
/// Submitting this form with the appropriate wifi and iop credentials will authenticate you
/// So the server should be closed;
Result<Option<AuthToken>, ServeError> CredentialsServer::serve(Option<struct station_config> storedWifi, Option<AuthToken> authToken) {
  this->start();

  const auto now = millis();
  if (!this->api->isConnected() && storedWifi.isSome() && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const auto stored = storedWifi.expect(STATIC_STRING("storedWifi is None"));
    const auto status = authenticateWifi(this->logger, *this->api, (char*) stored.ssid, (char*) stored.password);
    if (status == STATION_GOT_IP) {
      this->nextTryFlashWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      return Result<Option<AuthToken>, ServeError>(ServeError::REMOVE_WIFI_CONFIG);
    }
  }

  if (!this->api->isConnected() && wifiNetworkName.isSome() && wifiPassword.isSome() && this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
    const auto toString = [](const StaticString & str) { return String(str.get()); };
    const auto ssid = wifiNetworkName.map<String>(toString).expect(STATIC_STRING("Wifi name is None"));
    const auto password = wifiPassword.map<String>(toString).expect(STATIC_STRING("Wifi password is None"));
    const auto status = authenticateWifi(this->logger, *this->api, ssid, password);
    if (status == STATION_GOT_IP) {
      this->nextTryHardcodedWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = now + 24 * 3600 * 1000;
    }
  }

  if (this->api->isConnected() && authToken.isNone() && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;
    if (iopEmail.isSome() && iopPassword.isSome()) {
      const auto toString = [](const StaticString & str) { return String(str.get()); };
      const auto email = iopEmail.map<String>(toString).expect(STATIC_STRING("Iop email is None"));
      const auto password = iopPassword.map<String>(toString).expect(STATIC_STRING("Iop password is None"));
      auto token = authenticateIop(this->logger, *this->api, email, password);
      if (token.isSome()) {
        this->nextTryHardcodedIopCredentials = 0;
        return Result<Option<AuthToken>, ServeError>(std::move(token));
      }
    }
  }

  this->dnsServer.asMut()
    .expect(STATIC_STRING("DNSServer is none but shouldn't"))
    .get()->processNextRequest();
  this->server.asMut()
    .expect(STATIC_STRING("Server is none but shouldn't"))
    .get()->handleClient();
  return Result<Option<AuthToken>, ServeError>(Option<AuthToken>());
}
#endif

#ifdef IOP_SERVER_DISABLED
  Result<Option<AuthToken>, ServeError> CredentialsServer::serve(Option<struct station_config> storedWifi, Option<AuthToken> authToken) {
    (void) storedWifi;
    return Result<Option<AuthToken>, ServeError>(std::move(authToken));
  }
  void CredentialsServer::close() {}
  void CredentialsServer::start() {}
#endif
