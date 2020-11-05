#include <server.hpp>
#include <flash.hpp>
#include <api.hpp>
#include <log.hpp>
#include <configuration.h>
#include <network.hpp>

#include <WiFiClient.h>

const unsigned long intervalTryHardcodedWifiCredentialsMillis = 600000; // 10 minutes
const unsigned long intervalTryHardcodedMonitorCredentialsMillis = 3600000; // 1 hour

void MonitorCredentialsServer::start() {
  if (server.isNone()) {
    WiFiServer s = WiFiServer(80);
    s.begin();
    s.setNoDelay(false);
    server = Option<WiFiServer>(s);
  }
}

void MonitorCredentialsServer::close() {
  if (server.isSome()) {
    server.expect("Inside MonitorCredentialsServer::close, server is None but shouldn't be")
      .close();
    server = Option<WiFiServer>();
  }
}

bool MonitorCredentialsServer::authenticate(const String username, const String password) {
  const Option<AuthToken> authToken = api.authenticate(username, password);
  if (authToken.isSome()) {
    this->close();
    flash.writeAuthToken(authToken.expect("Inside calling writeAuthToken, token is None but shouldn't be"));
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
        return;
      }
    }
  }

  if (server.isSome()) {
    WiFiClient client = server.expect("Inside MonitorCredentialsServer::serve, server is None but shouldn't be").available();
    if (client && client.available()) {
      const String request = client.readStringUntil('\n');
      if (request.startsWith("GET")) {
        const String response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<!DOCTYPE HTML>\r\n"
          "<html><body>\r\n"
          "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
          "  <h3>Please provide your IOP credentials, so we can get a authentication token to use</h3>\r\n"
          "  <form style='margin: 0 auto; width: 300px;' action='/' method='POST'>\r\n"
          "    <div><label for='username'>Username:</label><input name='username' type='text' /></div>\r\n"
          "    <div><label for='password'>Password:</label><input name='password' type='password' /></div>\r\n"
          "    <input type='submit' value='Submit' />"
          "  </form>"
          "</body></html>";
        client.println(response);
        client.flush();
        client.stop();
      } else if (request.startsWith("POST")) {
        String data;
        while (!(data = client.readStringUntil('\n')).isEmpty()) {
          logger.debug(data);
        }
        Option<String> username = Option<String>();
        Option<String> password = Option<String>();
        do {
          data = client.readStringUntil('&');
          if (data.startsWith("username=")) {
            data.replace("username=", "");
            username = Option<String>(data);
          } else if (data.startsWith("password=")) {
            data.replace("password=", "");
            password = Option<String>(data);
          }
        } while (!data.isEmpty());

        if (username.isSome() && password.isSome()) {
          if (this->authenticate(username.expect("Username is None"), password.expect("Password is None"))) {
            client.println("Authentication succeeded :)");
            client.flush();
          } else {
            client.println("Authentication failed :(");
            client.flush();
          }
        }
        client.stop();
      } else {
        client.stop();
      }
    }
  }
}

void WifiCredentialsServer::start() {
  if (server.isNone()) {
    if ((WiFi.getMode() & WIFI_AP) == 0) {
      WiFi.mode(WIFI_AP_STA);
      // TODO: make u64 hash of the mac address and use it in the ssid
      // TODO: the password should be random, but also accessible externally (like a sticker in the hardware). The question is how???
      WiFi.softAP("iop-" + 4, "le$memester#passwordz");
    }

    WiFiServer s = WiFiServer(80);
    s.begin();
    s.setNoDelay(false);
    server = Option<WiFiServer>(s);
  }
}

void WifiCredentialsServer::close() {
  if (server.isSome()) {
    server.expect("Inside WifiCredentialsServer::close, server is None but shouldn't be")
      .close();
    server = Option<WiFiServer>();
  }
}

station_status_t WifiCredentialsServer::authenticate(const String ssid, const String password) {
  WiFi.begin(ssid, password);
  if (network.waitForConnection()) {
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
  if (this->nextTryHardcodedCredentials < now) {
    this->nextTryHardcodedCredentials = now + intervalTryHardcodedWifiCredentialsMillis;
    if (wifiNetworkName.isSome() && wifiPassword.isSome()) {
      // TODO: we could avoid this indefinitely if the
      // `wifi_station_get_connect_status()` status code is STATION_WRONG_PASSWORD
      const station_status_t status = this->authenticate(wifiNetworkName.expect("Wifi name is None"), wifiPassword.expect("Wifi password is None"));
      if (status == STATION_GOT_IP) {
        return;
      } else if (status == STATION_WRONG_PASSWORD) {
        this->nextTryHardcodedCredentials = now + 24 * 3600 * 1000;
      }
    }
  }

  // TODO: we should redirect all router traffic to this page
  // TODO: we should also (optionally) get iop credentials now, to avoid another configuration step
  if (server.isSome()) {
    WiFiClient client = server.expect("Inside WifiCredentialsServer::serve, server is None but shouldn't be").available();
    if (client && client.available()) {
      const String request = client.readStringUntil('\n');
      if (request.startsWith("GET")) {
        const String response =
          "HTTP/1.1 200 OK\r\n"
          "Content-Type: text/html\r\n"
          "\r\n"
          "<!DOCTYPE HTML>\r\n"
          "<html><body>\r\n"
          "  <h1><center>Hello, I'm your plantomator</center></h1>\r\n"
          "  <h3>Please provide your IOP credentials, so we can get a authentication token to use</h3>\r\n"
          "  <form style='margin: 0 auto; width: 300px;' action='/' method='POST'>\r\n"
          "    <div><label for='ssid'>Network name:</label><input name='ssid' type='text' /></div>\r\n"
          "    <div><label for='password'>Password:</label><input name='password' type='password' /></div>\r\n"
          "    <input type='submit' value='Submit' />"
          "  </form>"
          "</body></html>";
        client.println(response);
        client.flush();
        client.stop();
      } else if (request.startsWith("POST")) {
        String data;
        while (!(data = client.readStringUntil('\n')).isEmpty()) {
          logger.debug(data);
        }
        Option<String> ssid = Option<String>();
        Option<String> password = Option<String>();
        do {
          data = client.readStringUntil('&');
          if (data.startsWith("ssid=")) {
            data.replace("ssid=", "");
            ssid = Option<String>(data);
          } else if (data.startsWith("password=")) {
            data.replace("password=", "");
            password = Option<String>(data);
          }
        } while (!data.isEmpty());

        if (ssid.isSome() && password.isSome()) {
          const station_status_t status = this->authenticate(ssid.expect("SSID is None"), password.expect("Password is None"));
          if (status == STATION_GOT_IP) {
            client.println("Authentication succeeded :)");
          } else if (status == STATION_WRONG_PASSWORD) {
            client.println("Wrong password");
          } else {
            client.println("Authentication failed :(. Status: " + String(status));
          }
          client.flush();
        }
        client.stop();
      } else {
        client.stop();
      }
    }
  }
}
