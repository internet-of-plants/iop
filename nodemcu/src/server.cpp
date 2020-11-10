#include <server.hpp>

#ifndef IOP_SERVER_DISABLED
#include <configuration.h>
#include <utils.hpp>

#include <IPAddress.h>
#include <WiFiClient.h>

const unsigned long intervalTryFlashWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedIopCredentialsMillis = 3600000; // 1 hour

// TODO: add csrf protection

const char pageHTML[] PROGMEM =
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h4>To reset your configurations in the future, please press the reset button for at least 10 seconds</h4>"
    "<form style='margin: 0 auto; width: 500px;' action='/submit' method='POST'>\r\n"
    "%s"
    "<br>\r\n"
    "<input type='submit' value='Submit' />\r\n"
    "</form></body></html>";

const char wifiHTML[] PROGMEM =
    "<h3><center>Please provide your Wifi credentials, so we can connect to it</center></h3>\r\n"
    "<div><div><strong>Network name:</strong></div><input name='ssid' type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='password' type='password' style='width:100%' /></div>\r\n";

const char iopHTML[] PROGMEM =
    "<h3><center>Please provide your Iop credentials, so we can get a authentication token to use</center></h3>\r\n"
    "<div><div><strong>Email:</strong></div><input name='iopEmail' type='text' style='width:100%' /></div>\r\n"
    "<div><div><strong>Password:</strong></div><input name='iopPassword' type='password' style='width:100%' /></div>\r\n";

#include <unordered_map>

void CredentialsServer::start() {
  if (server.isNone()) {
    this->logger.info("Setting our own wifi access point");

    // TODO: the password should be random (passed at compile time)
    // But also accessible externally (like a sticker in the hardware).
    WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    WiFi.softAP("iop-" + hashString(WiFi.macAddress()), "le$memester#passwordz");

    // Makes it a captive portal (redirects all wifi trafic to us)
    auto dns = std::make_unique<DNSServer>();
    dns->setErrorReplyCode(DNSReplyCode::NoError);
    dns->start(53, "*", WiFi.softAPIP());

    auto s = std::make_shared<ESP8266WebServer>(80);
    s->on("/submit", [s, this]() {
      s->sendHeader("Connection", "close");
      if (s->hasArg("ssid") && s->hasArg("password")) {
        const station_status_t status = this->authenticateWifi(s->arg("ssid"), s->arg("password"));
        if (status == STATION_GOT_IP) {
          s->send(200, "text/plain", "Authentication succeeded :)");
        } else if (status == STATION_WRONG_PASSWORD) {
          s->send(403, "text/plain", "Wrong password");
        } else {
          s->send(404, "text/plain", "Authentication failed :(. Status: " + String(status));
        }
      }

      if (s->hasArg("iopEmail") && s->hasArg("iopPassword")) {
        this->authenticateIop(s->arg("iopEmail"), s->arg("iopPassword"));
      }
    });
    s->onNotFound([s, this]() {
      s->sendHeader("Connection", "close");
      String content = pageHTML;
      if (!this->api.isConnected()) {
        content += wifiHTML;
      }
      if (this->flash.readAuthToken().isNone()) {
        content += iopHTML;
      }
      s->send(200, "text/html", pageHTML.c_str());
    });
    s->begin();
    this->logger.info("Opened captive portal");
    this->logger.info(WiFi.softAPIp());
    server = std::move(s);
    dnsServer = std::move(dns);
  }
}

void CredentialsServer::close() {
  if (this->server.isSome()) {
    this->server.take().expect("Inside CredentialsServer::close, server is None but shouldn't be")->close();
  }
  if (this->dns.isSome()) {
    this->dnsServer.take().expect("Inside CredentialsServer::close, dnsServer is None but shouldn't be")->close();
  }
}

Option<AuthToken> CredentialsServer::authenticateIop(const String username, const String password) {
  auto authToken = this->api.authenticate(username, password);
  if (authToken.isSome()) {
    const AuthToken token = authToken.expect("Inside authenticateIop: Auth Token is none but shouldn't.");
    return Option<AuthToken>(token);
  }

  this->logger.warn("Invalid wifi credentials: " + username + ", " + password);
  return Option<AuthToken>();
}

station_status_t CredentialsServer::authenticateWifi(const String ssid, const String password) {
  const auto status = wifi_station_get_connect_status();
  if (status == STATION_CONNECTING) {
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
  }

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == -1) {
    this->logger.warn("Wifi authentication timed out");
    return STATION_CONNECT_FAIL;
  }

  const auto status = wifi_station_get_connect_status();
  if (!this->api.isConnected()) {
    this->logger.warn("Invalid credentials (" + String(status) + "): " + ssid + ", " + password);
  }
  return status;
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
  if (!this->api.isConnected() && storedWifi.isSome() && this->nextTryFlashWifiCredentials <= now) {
    this->nextTryFlashWifiCredentials = now + intervalTryFlashWifiCredentialsMillis;

    const auto stored = storedWifi.expect("storedWifi is None");
    const auto status = this->authenticateWifi((char*) stored.ssid, (char*) stored.password);
    if (status == STATION_GOT_IP) {
      this->nextTryFlashWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      return Result<Option<AuthToken>, ServeError>(ServeError::REMOVE_WIFI_CONFIG);
    }
  }
  
  if (!this->api.isConnected() && wifiNetworkName.isSome() && wifiPassword.isSome() && this->nextTryHardcodedWifiCredentials <= now) {
    this->nextTryHardcodedWifiCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
    
    const auto ssid = wifiNetworkName.asRef().expect("Wifi name is None");
    const auto password = wifiPassword.asRef().expect("Wifi password is None");
    const auto status = this->authenticateWifi(ssid, password);
    if (status == STATION_GOT_IP) {
      this->nextTryHardcodedWifiCredentials = 0;
    } else if (status == STATION_WRONG_PASSWORD) {
      this->nextTryHardcodedWifiCredentials = now + 24 * 3600 * 1000;
    }
  }

  if (this->api.isConnected() && authToken.isNone() && this->nextTryHardcodedIopCredentials <= now) {
    this->nextTryHardcodedIopCredentials = now + intervalTryHardcodedIopCredentialsMillis;
    if (iopEmail.isSome() && iopPassword.isSome()) {
      const auto email = iopEmail.asRef().expect("Iop email is None");
      const auto password = iopPassword.asRef().expect("Iop password is None");
      auto token = this->authenticateIop(email, password);
      if (token.isSome()) {
        this->nextTryHardcodedIopCredentials = 0;
        return Result<Option<AuthToken>, ServeError>(std::move(token));
      }
    }
  }

  if (this->server.isNone()) {
    panic_("Server is none but shouldn't be");
  }
  this->server.asRef().handleClient(); 
  this->dnsServer.asRef().processNextRequest();
  return Result<Option<AuthToken>, ServeError>(Option<AuthToken>());
}
#endif

#ifdef IOP_SERVER_DISABLED
  Option<AuthToken> CredentialsServer::authenticateIop(const String username, const String password) {
    (void) username;
    (void) password;
    return Option<AuthToken>({0});
  }
  station_status_t CredentialsServer::authenticateWifi(const String ssid, const String password) {
    (void) ssid;
    (void) password;
    return STATION_GOT_IP;
  }
  Result<Option<AuthToken>, ServeError> CredentialsServer::serve(Option<struct station_config> storedWifi, Option<AuthToken> authToken) {
    (void) storedWifi;
    return Result<Option<AuthToken>, ServeError>(std::move(authToken));
  }
  void CredentialsServer::close() {}
  void CredentialsServer::start() {}
#endif