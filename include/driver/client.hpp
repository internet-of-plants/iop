#ifndef IOP_DRIVER_CLIENT
#define IOP_DRIVER_CLIENT

#include <vector>
#include <string>
#include <unordered_map>
#include "core/string/static.hpp"

enum HTTPUpdateResult {
    HTTP_UPDATE_FAILED,
    HTTP_UPDATE_NO_UPDATES,
    HTTP_UPDATE_OK
};

class WiFiClient {
public:
  void setNoDelay(bool b) {}
  void setSync(bool b) {}
};

class HTTPClient {
  std::vector<std::string> headersToCollect;

  std::string uri;
  std::unordered_map<std::string, std::string> headers;
  std::string authorization;

  std::string responsePayload;
  std::unordered_map<std::string, std::string> responseHeaders;
public:
  void setReuse(bool reuse) {}
  void collectHeaders(const char **headerKeys, size_t count) {
    for (uint8_t index = 0; index < count; ++index) {
        this->headersToCollect.push_back(headerKeys[index]);
    }
  }
  std::string header(std::string key) {
    if (this->responseHeaders.count(key) != 0) return "";
    return this->responseHeaders.at(key);
  }
  size_t getSize() {
    return this->responsePayload.length();
  }
  std::string getString() {
    return this->responsePayload;
  }
  void end() {
    this->headers.clear();
    this->authorization.clear();
    this->responsePayload.clear();
    this->responseHeaders.clear();
    this->uri.clear();
  }
  void addHeader(iop::StaticString key, iop::StaticString value) {
    this->headers.emplace(key.toStdString(), std::move(value.toStdString()));
  }
  void addHeader(iop::StaticString key, std::string value) {
    this->headers.emplace(key.toStdString(), std::move(value));
  }
  void setTimeout(uint32_t ms) {}
  void setAuthorization(std::string auth) {
    this->authorization = auth;
  }
  int sendRequest(std::string method, const uint8_t *data, size_t len) {
    return 500;
  }
  bool begin(WiFiClient client, std::string uri) {
    this->end();
    this->uri = uri;
    (void) client;
    return false;
  }
};

#endif