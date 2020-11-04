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

void networkSetup();
bool isConnected();
bool waitForConnection();
Option<Response> httpPut(const String token, const String path, const String data);
Option<Response> httpPost(const String token, const String path, const String data);
Option<Response> httpPost(const String path, const String data);
Option<Response> httpRequest(const HttpMethod method, const Option<String> token, const String path, const Option<String> data);
bool connect();
String wifiCodeToString(const wl_status_t val);

#endif