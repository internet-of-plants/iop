#ifndef IOP_SERVER_H_
#define IOP_SERVER_H_

#include <utils.hpp>

#include <WiFiServer.h>

class WifiCredentialsServer {
  unsigned long nextTryHardcodedCredentials = 0;

  private:
    Option<WiFiServer> server;
  public:
    WifiCredentialsServer(): server(Option<WiFiServer>()) {}
    WifiCredentialsServer(const WiFiServer server): server(server) {}

    station_status_t authenticate(const String ssid, const String password);
    void serve();
    void close();
    void start();
};

class MonitorCredentialsServer {
  unsigned long lastTryHardcodedCredentials = 0;

  private:
    Option<WiFiServer> server;
  public:
    MonitorCredentialsServer(): server(Option<WiFiServer>()) {}
    MonitorCredentialsServer(const WiFiServer server): server(server) {}

    bool authenticate(const String username, const String password);
    void serve();
    void close();
    void start();
};

#endif