#include <server.hpp>
#include <flash.hpp>
#include <api.hpp>
#include <log.hpp>
#include <configuration.h>
#include <network.hpp>

#include <WiFiClient.h>
#include <DNSServer.h>

const unsigned long intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedMonitorCredentialsMillis = 3600000; // 1 hour

// TODO: form submitions should be encrypted, can we host using https?
// TODO: add csrf protection

const String iopHTML =
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h3><center>Please provide your Internet of Plants credentials, so we can get a authentication token to use</center></h3>\r\n"
    "  <form style='margin: 0 auto; width: 500px;' action='/submit' method='POST'>\r\n"
    "    <div><div><strong>Email:</strong></div><input name='iopEmail' type='text' style='width:100%' /></div>\r\n"
    "    <div><div><strong>Password:</strong></div><input name='iopPassword' type='password' style='width:100%' /></div>\r\n"
    "    <br>\r\n"
    "    <input type='submit' value='Submit' />\r\n"
    "  </form>\r\n"
    "</body></html>";

void MonitorCredentialsServer::start() {
  if (server.isNone()) {
    logger.info("Setting our own iop credentials retriever server");

    auto s = std::make_shared<ESP8266WebServer>(80);
    s->on("/submit", [s, this]() {
      s->sendHeader("Connection", "close");
      if (s->hasArg("iopEmail") && s->hasArg("iopPassword")) {
        if (this->authenticate(s->arg("iopEmail"), s->arg("iopPassword"))) {
          s->send(200, "text/plain", "IoP credentials authenticated!");
        }
      }
    });
    s->onNotFound([s]() {
      s->sendHeader("Connection", "close");
      s->send(200, "text/html", iopHTML.c_str());
    });
    s->begin();
    server = Option<std::shared_ptr<ESP8266WebServer>>(std::move(s));
  }
}

void MonitorCredentialsServer::close() {
  if (server.isSome()) {
    server.expect("Inside MonitorCredentialsServer::close, server is None but shouldn't be")->close();
    server = Option<std::shared_ptr<ESP8266WebServer>>();
  }
}

bool MonitorCredentialsServer::authenticate(const String username, const String password) {
  const auto authToken = api.authenticate(username, password);
  if (authToken.isSome()) {
    this->close();
    flash.writeAuthToken(authToken.expect("Calling writeAuthToken, token is None but shouldn't be"));
    return true;
  }

  logger.warn("Invalid credentials: " + username + ", " + password);
  return false;
}

/// Abstracts away token acquisition.
/// If there are hardcoded credentials this will use them
/// (and avoid a floods if they are invalid)
///
/// If there aren't it will serve a HTTP server at port 80 that provides a HTML form
/// Submitting this form with the appropriate Internet of Plants credentials
void MonitorCredentialsServer::serve() {
  this->start();

  const unsigned long now = millis();
  if (this->lastTryHardcodedCredentials == 0
      || (this->lastTryHardcodedCredentials + intervalTryHardcodedMonitorCredentialsMillis < now)) {

    this->lastTryHardcodedCredentials = now;
    if (iopEmail.isSome() && iopPassword.isSome()) {
      if (this->authenticate(iopEmail.expect("Iop email is None"), iopPassword.expect("Iop password is None"))) {
        this->lastTryHardcodedCredentials = 0;
        return;
      }
    }
  }

  // TODO set lastTryHardcodedCredentials to 0 when server obtains credentials
  if (server.isSome()) {
    server.expect("Inside MonitorCredentialsServer::serve, server is None but shouldn't be")->handleClient();
  }
}

const String wifiHTML =
    "<!DOCTYPE HTML>\r\n"
    "<html><body>\r\n"
    "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
    "  <h3><center>Please provide your Wifi credentials, so we can get a authentication token to use</center></h3>\r\n"
    "  <form style='margin: 0 auto; width: 500px;' action='/submit' method='POST'>\r\n"
    "    <div><div><strong>Network name:</strong></div><input name='ssid' type='text' style='width:100%' /></div>\r\n"
    "    <div><div><strong>Password:</strong></div><input name='password' type='password' style='width:100%' /></div>\r\n"
    "    <h3>You may also provide your Internet of Plants monitor credentials and skip a configuration step</h3>\r\n"
    "    <div><div><strong>Email:</strong></div><input name='iopEmail' type='text' style='width:100%' /></div>\r\n"
    "    <div><div><strong>Password:</strong></div><input name='iopPassword' type='password' style='width:100%' /></div>\r\n"
    "    <br>\r\n"
    "    <input type='submit' value='Submit' />\r\n"
    "  </form>\r\n"
    "</body></html>";

void WifiCredentialsServer::start() {
  if (server.isNone()) {
    logger.info("Setting our own wifi access point");

    // TODO: make u64 hash of the mac address and use it in the ssid
    // TODO: the password should be random, but also accessible externally (like a sticker in the hardware). The question is how???
    WiFi.softAPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    WiFi.softAP("iop-" + String(4), "le$memester#passwordz");
    delay(500);

    auto s = std::make_shared<ESP8266WebServer>(80);
    s->on("/submit", [s, this]() {
      s->sendHeader("Connection", "close");
      if (s->hasArg("ssid") && s->hasArg("password")) {
        const station_status_t status = this->authenticate(s->arg("ssid"), s->arg("password"));
        if (status == STATION_GOT_IP) {
          s->send(200, "text/plain", "Authentication succeeded :)");
        } else if (status == STATION_WRONG_PASSWORD) {
          s->send(403, "text/plain", "Wrong password");
        } else {
          s->send(404, "text/plain", "Authentication failed :(. Status: " + String(status));
        }
      }

      if (s->hasArg("iopEmail") && s->hasArg("iopPassword")) {
        const Option<AuthToken> authToken = api.authenticate(s->arg("iopEmail"), s->arg("iopPassword"));
        if (authToken.isSome()) {
          s->send(200, "text/plain", "IoP credentials authenticated!");
          flash.writeAuthToken(authToken.expect("Inside calling writeAuthToken, token is None but shouldn't be"));
        }
      }
    });
    s->onNotFound([s]() {
      s->sendHeader("Connection", "close");
      s->send(200, "text/html", wifiHTML.c_str());
    });
    s->begin();
    server = Option<std::shared_ptr<ESP8266WebServer>>(std::move(s));
  }
}

void WifiCredentialsServer::close() {
  if (server.isSome()) {
    server.expect("Inside WifiCredentialsServer::close, server is None but shouldn't be")->close();
    WiFi.softAPdisconnect();
    server = Option<std::shared_ptr<ESP8266WebServer>>();
  }
}

station_status_t WifiCredentialsServer::authenticate(const String ssid, const String password) {
  if (ssid.isEmpty() || password.isEmpty()) {
    // We pass wrong password for invalid ssid so it's treated as wrong instead of missing
    // Since we can't differentiate a network ssid that doesn't exist
    // from one that is just offline right now
    return STATION_WRONG_PASSWORD;
  }

  WiFi.begin(ssid, password);
  WiFi.waitForConnectResult();
  if (network.isConnected()) {
    this->close();
    return wifi_station_get_connect_status();
  }
  logger.warn("Invalid credentials: " + ssid + ", " + password);
  return wifi_station_get_connect_status();
}

/// Abstracts away wifi credentials acquisition.
/// If there are hardcoded credentials this will use them
/// (and avoid a floods if they are invalid)
///
/// If there aren't it will serve a HTTP server at port 80 that provides a HTML form
/// Submitting this form with the appropriate wifi credentials
void WifiCredentialsServer::serve() {
  this->start();

  const unsigned long now = millis();
  if (this->nextTryHardcodedCredentials <= now) {
    this->nextTryHardcodedCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
    if (wifiNetworkName.isSome() && wifiPassword.isSome()) {
      const station_status_t status = this->authenticate(wifiNetworkName.expect("Wifi name is None"), wifiPassword.expect("Wifi password is None"));
      if (status == STATION_GOT_IP) {
        return;
      } else if (status == STATION_WRONG_PASSWORD) {
        this->nextTryHardcodedCredentials = now + 24 * 3600 * 1000;
      }
    }
  }

  // TODO: we should redirect all router traffic to this page
  if (server.isSome()) {
    server.expect("Inside WiFiCredentialsServer::serve, server is None")->handleClient();
  }
}
