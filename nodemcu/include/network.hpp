#ifndef IOP_NETWORK_H_
#define IOP_NETWORK_H_

#include <utils.hpp>
#include <ESP8266WiFi.h>

typedef struct response {
  int code;
  Option<String> payload;
} Response;

enum HttpMethod {
  GET,
  DELETE,
  POST,
  PUT
};

class Network {
  public:
    Network() {}
    void setup() const;
    bool isConnected() const;
    bool waitForConnection() const;
    Option<Response> httpPut(const String token, const String path, const String data) const;
    Option<Response> httpPost(const String token, const String path, const String data) const;
    Option<Response> httpPost(const String path, const String data) const;
    Option<Response> httpRequest(const HttpMethod method, const Option<String> token, const String path, const Option<String> data) const;
    bool connect() const;
    String wifiCodeToString(const wl_status_t val) const;
};

const static Network network = Network();

#endif