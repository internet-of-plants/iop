#ifndef IOP_SERVER_H_
#define IOP_SERVER_H_

#include <utils.hpp>

#include <ESP8266WebServer.h>

class WifiCredentialsServer {
  private:
    unsigned long nextTryHardcodedCredentials = 0;
    Option<std::shared_ptr<ESP8266WebServer>> server;

  public:
    WifiCredentialsServer(): server(Option<std::shared_ptr<ESP8266WebServer>>()) {}
    WifiCredentialsServer(const std::shared_ptr<ESP8266WebServer> server): server(server) {}

    station_status_t authenticate(const String ssid, const String password);
    void serve();
    void close();
    void start();
};

class MonitorCredentialsServer {
  private:
    Option<std::shared_ptr<ESP8266WebServer>> server;
    unsigned long lastTryHardcodedCredentials = 0;

  public:
    MonitorCredentialsServer(): server(Option<std::shared_ptr<ESP8266WebServer>>()) {}
    MonitorCredentialsServer(const std::shared_ptr<ESP8266WebServer> server): server(server) {}

    bool authenticate(const String username, const String password);
    void serve();
    void close();
    void start();
};

#endif