#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "driver/network.hpp"
#include "ESP8266HTTPClient.h"
#include "WiFiClientSecure.h"
#include <charconv>
#include <system_error>

namespace driver {
Session::Session(HTTPClient &http, std::string uri) noexcept: http_(&http), uri_(std::move(uri)) { IOP_TRACE(); }
Session::~Session() noexcept {
  IOP_TRACE();
  if (this->http_ && this->http_->http)
    this->http_->http->end();
}
Session::Session(Session&& other) noexcept: http_(other.http_), uri_(std::move(other.uri_)) {
  IOP_TRACE();
  other.http_ = nullptr;
}
auto Session::operator=(Session&& other) noexcept -> Session & {
  IOP_TRACE();
  this->http_ = other.http_;
  other.http_ = nullptr;
  this->uri_ = std::move(other.uri_);
  return *this;
}
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept {
  iop_assert(this->http, FLASH("HTTP client is nullptr"));
  this->http->collectHeaders(headers, count);
}
std::string Response::header(iop::StaticString key) const noexcept {
  const auto value = this->headers_.find(key.toString());
  if (value == this->headers_.end()) return "";
  return std::move(value->second);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept {
  iop_assert(this->http_ && this->http_->http, FLASH("Session has been moved out"));
  this->http_->http->addHeader(String(key.get()), String(value.get()));
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept {
  iop_assert(this->http_ && this->http_->http, FLASH("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());
  this->http_->http->addHeader(String(key.get()), val);
}
void Session::setAuthorization(std::string auth) noexcept {
  iop_assert(this->http_ && this->http_->http, FLASH("Session has been moved out"));
  this->http_->http->setAuthorization(auth.c_str());
}
auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> std::variant<Response, int> {
  iop_assert(this->http_ && this->http_->http, FLASH("Session has been moved out"));
  const auto code = this->http_->http->sendRequest(method.c_str(), data, len);
  if (code < 0) {
    return code;
  }

  std::unordered_map<std::string, std::string> headers;
  headers.reserve(this->http_->http->headers());
  for (int index = 0; index < this->http_->http->headers(); ++index) {
    const auto key = std::string(this->http_->http->headerName(index).c_str());
    const auto value = std::string(this->http_->http->header(index).c_str());
    headers[key] = value;
  }

  const auto httpString = this->http_->http->getString();
  const auto http_string = std::string(httpString.c_str());
  const auto payload = Payload(http_string);
  const auto response = Response(headers, payload, code);
  return response;
}

HTTPClient::HTTPClient() noexcept: http(new (std::nothrow) ::HTTPClient()) {
  iop_assert(http, FLASH("OOM"));
}
HTTPClient::~HTTPClient() noexcept {
  delete this->http;
}

std::optional<Session> HTTPClient::begin(std::string uri) noexcept {
  IOP_TRACE(); 
  
  iop_assert(iop::data.wifi.client, FLASH("Wifi has been moved out, client is nullptr"));
  //iop::data.wifi.client.setNoDelay(false);
  //iop::data.wifi.client.setSync(true);

  // We can afford bigger timeouts since we shouldn't make frequent requests
  //constexpr uint16_t oneMinuteMs = 3 * 60 * 1000;
  //this->http.setTimeout(oneMinuteMs);

  //this->http.setAuthorization("");

  // Parse URI
  auto index = uri.find("://");
  if (index == uri.npos) {
    index = 0;
  } else {
    index += 3;
  }

  auto host = std::string_view(uri).substr(index);
  host = host.substr(0, host.find('/'));

  auto portStr = std::string_view();
  index = host.find(':');

  if (index != host.npos) {
    portStr = host.substr(index + 1);
    host = host.substr(0, index);
  } else if (uri.find("http://") != uri.npos) {
    portStr = "80";
  } else if (uri.find("https://") != uri.npos) {
    portStr = "443";
  } else {
    iop_panic(FLASH("Protocol missing inside HttpClient::begin"));
  }

  uint16_t port;
  auto result = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
  if (result.ec != std::errc()) {
    iop_panic(FLASH("Unable to confert port to uint16_t: ").toString() + portStr.begin() + FLASH(" ").toString() + std::error_condition(result.ec).message());
  }

  iop_assert(iop::data.wifi.client, FLASH("Wifi has been moved out, client is nullptr"));
  if (!iop::data.wifi.client->connect(std::string(host).c_str(), port)) {
    return std::nullopt;
  }
  
  iop_assert(this->http, FLASH("HTTP client is nullptr"));
  //this->http.setReuse(false);
  if (this->http->begin(*iop::data.wifi.client, String(uri.c_str()))) {
    return Session(*this, uri);
  }

  return std::nullopt;
}
}