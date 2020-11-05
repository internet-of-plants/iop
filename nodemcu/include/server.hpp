#ifndef IOP_SERVER_H_
#define IOP_SERVER_H_

#include <utils.hpp>

#include <WiFiServer.h>

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