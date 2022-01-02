#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "driver/network.hpp"
#include "ESP8266HTTPClient.h"
#include "WiFiClientSecure.h"
#include <charconv>
#include <system_error>

namespace driver {

auto rawStatus(const int code) noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;
  case HTTPC_ERROR_CONNECTION_FAILED:
    return RawStatus::CONNECTION_FAILED;
  case HTTPC_ERROR_SEND_HEADER_FAILED:
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
    return RawStatus::SEND_FAILED;
  case HTTPC_ERROR_NOT_CONNECTED:
  case HTTPC_ERROR_CONNECTION_LOST:
    return RawStatus::CONNECTION_LOST;
  case HTTPC_ERROR_NO_STREAM:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_NO_HTTP_SERVER:
    return RawStatus::NO_SERVER;
  case HTTPC_ERROR_ENCODING:
    // Unsupported Transfer-Encoding header, if set it must be "chunked"
    return RawStatus::ENCODING_NOT_SUPPORTED;
  case HTTPC_ERROR_STREAM_WRITE:
    return RawStatus::READ_FAILED;
  case HTTPC_ERROR_READ_TIMEOUT:
    return RawStatus::READ_TIMEOUT;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

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
  iop_assert(this->http, IOP_STATIC_STRING("HTTP client is nullptr"));
  this->http->collectHeaders(headers, count);
}
std::string Response::header(iop::StaticString key) const noexcept {
  const auto value = this->headers_.find(key.toString());
  if (value == this->headers_.end()) return "";
  return std::move(value->second);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept {
  iop_assert(this->http_ && this->http_->http, IOP_STATIC_STRING("Session has been moved out"));
  this->http_->http->addHeader(String(key.get()), String(value.get()));
}
void Session::addHeader(iop::StaticString key, iop::StringView value) noexcept {
  iop_assert(this->http_ && this->http_->http, IOP_STATIC_STRING("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());
  this->http_->http->addHeader(String(key.get()), val);
}
void Session::setAuthorization(std::string auth) noexcept {
  iop_assert(this->http_ && this->http_->http, IOP_STATIC_STRING("Session has been moved out"));
  this->http_->http->setAuthorization(auth.c_str());
}
auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> Response {
  iop_assert(this->http_ && this->http_->http, IOP_STATIC_STRING("Session has been moved out"));
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
  iop_assert(http, IOP_STATIC_STRING("OOM"));
}
HTTPClient::~HTTPClient() noexcept {
  delete this->http;
}

std::optional<Session> HTTPClient::begin(std::string uri) noexcept {
  IOP_TRACE(); 
  
  //iop_assert(iop::data.wifi.client, IOP_STATIC_STRING("Wifi has been moved out, client is nullptr"));
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

  auto host = iop::StringView(uri).substr(index);
  host = host.substr(0, host.find('/'));

  auto portStr = iop::StringView();
  index = host.find(':');

  if (index != host.npos) {
    portStr = host.substr(index + 1);
    host = host.substr(0, index);
  } else if (uri.find("http://") != uri.npos) {
    portStr = "80";
  } else if (uri.find("https://") != uri.npos) {
    portStr = "443";
  } else {
    iop_panic(IOP_STATIC_STRING("Protocol missing inside HttpClient::begin"));
  }

  uint16_t port;
  auto result = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
  if (result.ec != std::errc()) {
    iop_panic(IOP_STATIC_STRING("Unable to confert port to uint16_t: ").toString() + portStr.begin() + IOP_STATIC_STRING(" ").toString() + std::error_condition(result.ec).message());
  }

  iop_assert(iop::data.wifi.client, IOP_STATIC_STRING("Wifi has been moved out, client is nullptr"));
  iop_assert(this->http, IOP_STATIC_STRING("HTTP client is nullptr"));
  //this->http.setReuse(false);
  if (this->http->begin(*iop::data.wifi.client, String(uri.c_str()))) {
    return Session(*this, uri);
  }

  return std::nullopt;
}
}