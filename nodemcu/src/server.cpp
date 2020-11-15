#include <server.hpp>

#ifndef IOP_SERVER_DISABLED
#include <configuration.h>
#include <utils.hpp>

#include <string>
#include <string.h>
#include <bits/basic_string.h>
#include <bits/basic_string.h>
#include <IPAddress.h>
#include <WiFiClient.h>

// TODO: make sure this captive portal can't be bypassed

const unsigned long intervalTryFlashWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedIopCredentialsMillis = 3600000; // 1 hour

const char pageHTMLStart[] PROGMEM =
  "<!DOCTYPE HTML>\r\n"
  "<html><body>\r\n"
  "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
  "  <h4><center>If, in the future, you want to reset the configurations set here, just press the factory reset button for at least 15 seconds</center></h4>"
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

Option<std::pair<String, String>> credentialsWifi;
Option<std::pair<String, String>> credentialsIop;

void CredentialsServer::start() {
  if (server.isNone()) {
    this->logger.info(F("Setting our own wifi access point"));

    WiFi.mode(WIFI_AP_STA);

    // TODO: the password should be random (passed at compile time)
    // But also accessible externally (like a sticker in the hardware). So not dynamic.
    WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    const auto hash = std::to_string(utils::hashString(this->api->macAddress()));
    const auto ssid = String("iop-") + String(hash.c_str());
    WiFi.setAutoReconnect(false);
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
    WiFi.softAP(ssid, "le$memester#passwordz", 2);
    WiFi.setAutoReconnect(true);
    WiFi.begin();
    // Makes it a captive portal (redirects all wifi trafic to us)
    auto dns = std::unique_ptr<DNSServer>(new DNSServer());
    dns->setErrorReplyCode(DNSReplyCode::NoError);
    dns->start(53, F("*"), WiFi.softAPIP());

    const auto api = this->api;
    const auto loggerLevel = this->logger.level();
    const auto flash = this->flash;

    auto s = std::make_shared<ESP8266WebServer>(80);
    s->on(F("/submit"), [s, loggerLevel]() {
      const Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
      logger.debug(F("Received form with credentials"));

      if (s->hasArg(F("ssid")) && s->hasArg(F("password"))) {
        credentialsWifi = std::pair<String, String>(s->arg(F("ssid")), s->arg(F("password")));
      }

      if (s->hasArg(F("iopEmail")) && s->hasArg(F("iopPassword"))) {
        credentialsIop = std::pair<String, String>(s->arg(F("iopEmail")), s->arg(F("iopPassword")));
      }
      s->sendHeader(F("Location"), F("/"));
      s->send_P(302, PSTR("text/plain"), PSTR(""));
    });
    s->onNotFound([s, api, flash, loggerLevel]() {
      const Log logger(loggerLevel, F("SERVER_CALLBACK"), false);
      logger.debug(F("Serving captive portal HTML"));

      const auto wifi = !api->isConnected();
      const auto iop = flash->readAuthToken().isNone();

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
    this->logger.info(F("Opened captive portal"));
    this->logger.info(WiFi.softAPIP().toString());
    server = std::move(s);
    dnsServer = std::move(dns);
  }
}

void CredentialsServer::close() {
  if (this->server.isSome()) {
    this->server.take().expect(F("Inside CredentialsServer::close, server is None but shouldn't be"))->close();
    WiFi.mode(WIFI_STA);
  }
  if (this->dnsServer.isSome()) {
    this->dnsServer.take().expect(F("Inside CredentialsServer::close, dnsServer is None but shouldn't be"))->stop();
  }
}

station_status_t CredentialsServer::connect(const StringView ssid, const StringView password) const {
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
      this->logger.warn(F("Invalid wifi credentials ("), START, F(" "));
      this->logger.warn(String(status), CONTINUITY, F("): "));
      this->logger.warn(ssid, CONTINUITY, F(", "));
      this->logger.warn(password, CONTINUITY, F("["));
    }

    if (status == STATION_WRONG_PASSWORD) {
      this->logger.warn(F("Wifi password was incorrect"));
    }
    return status;
  }
}

Result<AuthToken, Option<HttpCode>> CredentialsServer::authenticate(const StringView username, const StringView password) const {
    auto authToken = this->api->authenticate(username, password);
    if (authToken.isErr()) {
      auto maybeCode = authToken.expectErr(F("authToken is ok CredentialsServer::authenticate"));
      if (maybeCode.isSome()) {
        this->logger.warn(F("Invalid IoP credentials:"), START, F(" "));
        const auto code = maybeCode.expect(F("maybeCode is none Credentials::authenticate"));
        this->logger.warn(String(code), CONTINUITY, F(", "));
        this->logger.warn(username, CONTINUITY, F(", "));
        this->logger.warn(password, CONTINUITY);
      }
    }

    return std::move(authToken);
}

Result<Option<AuthToken>, ServeError> CredentialsServer::serve(const Option<struct WifiCredentials> & storedWifi, const Option<AuthToken> & authToken) {
  this->start();

  if (credentialsWifi.isSome()) {
    const auto cred = credentialsWifi.expect(F("credentialsWifi are none, inside CredentialsServer::serve"));
    this->connect(cred.first, cred.second);
  }

  if (credentialsIop.isSome()) {
    const auto cred = credentialsIop.expect(F("credentialsWifi are none, inside CredentialsServer::serve"));
    auto result = this->authenticate(cred.first, cred.second);
    if (result.isOk()) {
      const auto token = result.expectOk(F("result is Err but shouldn't"));
      return Result<Option<AuthToken>, ServeError>(token);
    }
  }

  const auto now = millis();
  if (!this->api->isConnected() && storedWifi.isSome() && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const struct WifiCredentials& stored = storedWifi.asRef().expect(F("storedWifi is None"));
    const auto status = this->connect(stored.ssid.asString(), stored.password.asString());
    if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = 0;
      return Result<Option<AuthToken>, ServeError>(ServeError::REMOVE_WIFI_CONFIG);
    }
  }

  if (!this->api->isConnected() && wifiNetworkName.isSome() && wifiPassword.isSome() && this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
    const StaticString& ssid = wifiNetworkName.asRef().expect(F("Wifi name is None"));
    const StaticString& password = wifiPassword.asRef().expect(F("Wifi password is None"));
    const auto status = this->connect(ssid, password);
    if (status == STATION_GOT_IP) {
      this->nextTryHardcodedWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = now + 24 * 3600 * 1000;
    }
  }

  if (this->api->isConnected() && authToken.isNone() && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;
    if (iopEmail.isSome() && iopPassword.isSome()) {
      const StaticString& email = iopEmail.asRef().expect(F("Iop email is None"));
      const StaticString& password = iopPassword.asRef().expect(F("Iop password is None"));
      auto token = this->authenticate(email, password);
      if (token.isOk()) {
        this->nextTryHardcodedIopCredentials = 0;
        return Result<Option<AuthToken>, ServeError>(token.expectOk(F("token is Err")));
      } else if (token.isErr()) {
        auto maybeCode = token.expectErr(F("token is Ok when it shouldn't"));
        if (maybeCode.isSome()) {
          const auto code = maybeCode.expect(F("maybeCode is none"));
          if (code == 403) {
            this->nextTryHardcodedIopCredentials = now + 24 + 3600 + 1000;
          }
        }
      } else {
        panic_(F("Result was empty"));
      }
    }
  }

  this->dnsServer.asMut()
    .expect(F("DNSServer is none but shouldn't"))
    .get()->processNextRequest();
  this->server.asMut()
    .expect(F("Server is none but shouldn't"))
    .get()->handleClient();
  return Result<Option<AuthToken>, ServeError>(Option<AuthToken>());
}
#endif

#ifdef IOP_SERVER_DISABLED
  Result<Option<AuthToken>, ServeError> CredentialsServer::serve(const Option<struct WifiCredentials> & storedWifi, const Option<AuthToken> & authToken) {
    (void) storedWifi;
    return Result<Option<AuthToken>, ServeError>(authToken.asRef()
      .map<AuthToken>([](const std::reference_wrapper<const AuthToken> token) { return token.get(); }));
  }
  void CredentialsServer::close() {}
  void CredentialsServer::start() {}
#endif
