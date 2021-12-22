#include "driver/client.hpp"
#include "core/string.hpp"
#include "core/log.hpp"
#include "core/data.hpp"

namespace iop {
  Data data;
}

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

auto rawStatusToString(const RawStatus status) noexcept -> iop::StaticString {
  IOP_TRACE();
  switch (status) {
  case driver::RawStatus::CONNECTION_FAILED:
    return F("CONNECTION_FAILED");
  case driver::RawStatus::SEND_FAILED:
    return F("SEND_FAILED");
  case driver::RawStatus::READ_FAILED:
    return F("READ_FAILED");
  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
    return F("ENCODING_NOT_SUPPORTED");
  case driver::RawStatus::NO_SERVER:
    return F("NO_SERVER");
  case driver::RawStatus::READ_TIMEOUT:
    return F("READ_TIMEOUT");
  case driver::RawStatus::CONNECTION_LOST:
    return F("CONNECTION_LOST");
  case driver::RawStatus::OK:
    return F("OK");
  case driver::RawStatus::SERVER_ERROR:
    return F("SERVER_ERROR");
  case driver::RawStatus::FORBIDDEN:
    return F("FORBIDDEN");
  case driver::RawStatus::UNKNOWN:
    return F("UNKNOWN");
  }
  return F("UNKNOWN-not-documented");
  }
}

#ifndef IOP_DESKTOP
#include <loop.hpp>
#include "WString.h"

#include <charconv>

namespace driver {
Session::Session(HTTPClient &http, std::string uri) noexcept: http_(&http), uri_(std::move(uri)) { IOP_TRACE(); }
Session::~Session() noexcept {
  IOP_TRACE();
  if (this->http_)
    this->http_->http.end();
}
Session::Session(Session&& other) noexcept: http_(other.http_), uri_(std::move(other.uri_)) {
  IOP_TRACE();
  other.http_ = nullptr;
}
auto Session::operator=(Session&& other) noexcept {
  IOP_TRACE();
  this->http_ = other.http_;
  other.http_ = nullptr;
  this->uri_ = std::move(other.uri_);
}
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept {
  return this->http.collectHeaders(headers, count);
}
std::string Response::header(iop::StaticString key) const noexcept {
  const auto value = this->headers_.find(key.toString());
  if (value == this->headers_.end()) return "";
  return std::move(value->second);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept {
  iop_assert(this->http_, F("Session has been moved out"));
  this->http_->http.addHeader(String(key.get()), String(value.get()));
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept {
  iop_assert(this->http_, F("Session has been moved out"));
  String val;
  val.concat(value.begin(), value.length());
  this->http_->http.addHeader(String(key.get()), val);
}
void Session::setAuthorization(std::string auth) noexcept {
  iop_assert(this->http_, F("Session has been moved out"));
  this->http_->http.setAuthorization(auth.c_str());
}
auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> std::variant<Response, int> {
  iop_assert(this->http_, F("Session has been moved out"));
  const auto code = this->http_->http.sendRequest(method.c_str(), data, len);
  if (code < 0) {
    return code;
  }

  std::unordered_map<std::string, std::string> headers;
  headers.reserve(this->http_->http.headers());
  for (int index = 0; index < this->http_->http.headers(); ++index) {
    const auto key = std::string(this->http_->http.headerName(index).c_str());
    const auto value = std::string(this->http_->http.header(index).c_str());
    headers[key] = value;
  }

  const auto httpString = this->http_->http.getString();
  const auto http_string = std::string(httpString.c_str());
  const auto payload = Payload(http_string);
  const auto response = Response(headers, payload, code);
  return response;
}
std::optional<Session> HTTPClient::begin(std::string uri) noexcept {
  IOP_TRACE();
  //this->http.setReuse(false);
  
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
    iop_panic(F("Protocol missing inside HttpClient::begin"));
  }

  uint16_t port;
  auto result = std::from_chars(portStr.data(), portStr.data() + portStr.size(), port);
  if (result.ec != std::errc()) {
    iop_panic(iop::StaticString(F("Unable to confert port to uint16_t: ")).toString() + portStr.begin() + iop::StaticString(F(" ")).toString() + std::error_condition(result.ec).message());
  }

  if (!iop::data.wifi.client.connect(std::string(host).c_str(), port)) {
    return std::nullopt;
  }
  
  if (this->http.begin(iop::data.wifi.client, String(uri.c_str()))) {
    return Session(*this, uri);
  }

  return std::nullopt;
}
}
#else

#include "driver/wifi.hpp"
#include <core/panic.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include "core/string.hpp"
#include "core/utils.hpp"
#include "core/log.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory>

static iop::Log clientDriverLogger(iop::LogLevel::DEBUG, F("HTTP Client"));

namespace driver {
static ssize_t send(uint32_t fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing())
    iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  return write(fd, msg, len);
}

Session::Session(HTTPClient &http, std::string uri, int32_t fd) noexcept: http_(&http), uri_(std::move(uri)), fd_(fd) { IOP_TRACE(); }
Session::~Session() noexcept {
  if (this->fd_ != -1)
    close(this->fd_);
}
Session::Session(Session&& other) noexcept: http_(other.http_), uri_(std::move(other.uri_)), fd_(other.fd_) {
  IOP_TRACE();
  other.http_ = nullptr;
  other.fd_ = -1;
}
auto Session::operator=(Session&& other) noexcept {
  IOP_TRACE();
  this->http_ = other.http_;
  other.http_ = nullptr;

  this->uri_ = std::move(other.uri_);

  this->fd_ = other.fd_;
  other.fd_ = -1;
}
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept {
  std::vector<std::string> vec;
  vec.reserve(count);
  for (size_t index = 0; index <= count; ++index) {
    vec.push_back(headers[index]);
  }
  this->headersToCollect_ = headers;
}
std::string Response::header(iop::StaticString key) const noexcept {
  const auto keyString = key.toString();
  if (this->headers_.count(keyString) == 0) return "";
  return this->headers_.at(keyString);
}
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept  {
  auto keyLower = key.toString();
  // Headers can't be UTF8 so we cool
  std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
      [](unsigned char c){ return std::tolower(c); });
  this->headers.emplace(keyLower, std::move(value.toString()));
}
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept  {
  auto keyLower = key.toString();
  // Headers can't be UTF8 so we cool
  std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
      [](unsigned char c){ return std::tolower(c); });
  this->headers.emplace(keyLower, std::string(value));
}
void Session::setAuthorization(std::string auth) noexcept  {
  if (auth.length() == 0) return;
  this->headers.emplace(std::string("Authorization"), std::string("Basic ") + auth);
}

auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> std::variant<Response, int> {
  if (iop::data.wifi.status() != driver::StationStatus::GOT_IP) return static_cast<int>(RawStatus::CONNECTION_FAILED);

  std::unordered_map<std::string, std::string> responseHeaders;
  responseHeaders.reserve(this->http_->headersToCollect_.size());
  std::string responsePayload;

  const std::string_view path(this->uri_.c_str() + this->uri_.find("/", this->uri_.find("://") + 3));
  clientDriverLogger.debug(F("Send request to "), path);

  auto fd = this->fd_;
  iop_assert(fd != -1, F("Invalid file descriptor"));
  if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
    iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::START);
  send(fd, method.c_str(), method.length());
  send(fd, " ", 1);
  send(fd, path.begin(), path.length());
  send(fd, " HTTP/1.0\r\n", 11);
  send(fd, "Content-Length: ", 16);
  const auto dataLengthStr = std::to_string(len);
  send(fd, dataLengthStr.c_str(), dataLengthStr.length());
  send(fd, "\r\n", 2);
  for (const auto& [key, value]: this->headers) {
    send(fd, key.c_str(), key.length());
    send(fd, ": ", 2);
    send(fd, value.c_str(), value.length());
    send(fd, "\r\n", 2);
  }
  send(fd, "\r\n", 2);
  send(fd, (char*)data, len);
  if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
    iop::Log::print(F("\n"), iop::LogLevel::TRACE, iop::LogType::END);
  clientDriverLogger.debug(F("Sent data"));
  
  std::unique_ptr<std::array<char, 4096>> buffer = std::make_unique<std::array<char, 4096>>();
  
  ssize_t size = 0;
  auto firstLine = true;
  auto isPayload = false;
  
  auto status = std::make_optional(1000);
  std::string_view buff(buffer->begin());
  while (true) {
    clientDriverLogger.debug(F("Try read: "), buff);

    if (buff.length() < buffer->max_size() &&
        (size = read(fd, buffer->data() + buff.length(), buffer->max_size() - buff.length())) < 0) {
      clientDriverLogger.error(F("Error reading from socket ("), std::to_string(size), F("): "), std::to_string(errno), F(" - "), strerror(errno)); 
      close(fd);
      return static_cast<int>(RawStatus::CONNECTION_FAILED);
    }
    buff = buffer->begin();
    clientDriverLogger.debug(F("Len: "), std::to_string(size));
    if (firstLine && size == 0) {
      close(fd);
      clientDriverLogger.warn(F("Empty request: "), std::to_string(fd), F(" "), std::string(reinterpret_cast<const char*>(data), len));
      return Response(status.value_or(500));
      //continue;
    }
    
    clientDriverLogger.debug(F("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(F("\n"))) continue;
    clientDriverLogger.debug(F("Read: ("), std::to_string(size), F(") ["), std::to_string(buff.length()), F("]"));

    if (firstLine && buff.find("\n") == buff.npos) continue;

    if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
      clientDriverLogger.error(F("Error reading first line: "), std::to_string(size));
      return static_cast<int>(RawStatus::READ_FAILED);
    }

    if (firstLine && size > 0) {
      clientDriverLogger.debug(F("Found first line: "));

      const std::string_view statusStr(buffer->begin() + 9); // len("HTTP/1.1 ") = 9
      const auto codeEnd = statusStr.find(" ");
      if (codeEnd == statusStr.npos) {
        clientDriverLogger.error(F("Bad server: "), statusStr, F(" -- "), buff);
        return static_cast<int>(RawStatus::READ_FAILED);
      }
      //iop_assert(buff.contains(F("\n")), iop::StaticString(F("First: ")).toString() + std::to_string(buffer.length()) + iop::StaticString(F(" bytes don't contain newline, the path is too long\n")).toString());
      status = atoi(std::string(statusStr.begin(), 0, codeEnd).c_str());
      clientDriverLogger.debug(F("Status: "), std::to_string(status.value_or(500)));
      firstLine = false;

      const char* ptr = buff.begin() + buff.find("\n") + 1;
      memmove(buffer->data(), ptr, strlen(ptr) + 1);
      buff = buffer->begin();
    }
    if (!status) {
      clientDriverLogger.error(F("No status"));
      return static_cast<int>(RawStatus::READ_FAILED);
    }
    clientDriverLogger.debug(F("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(F("\n"))) continue;
    //clientDriverLogger.debug(F("Headers + Payload: "), buff.substr(0, buff.find("\n")));

    while (len > 0 && buff.length() > 0 && !isPayload) {
      // TODO: if empty line is split into t  wo reads (because of buff len) we are screwed
      //  || buff.contains(F("\n\n")) || buff.contains(F("\n\r\n"))
      if (buff.find("\r\n") == 0) {
        // TODO: move this logic to inside Response::await to be lazy-evaluated
        clientDriverLogger.debug(F("Found Payload"));
        isPayload = true;

        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer->data(), ptr, strlen(ptr) + 1);
        buff = buffer->begin();
        continue;
      } else if (buff.find("\r\n") == buff.npos) {
        iop_panic(F("Bad software bruh"));
      } else if (buff.find("\r\n") != buff.npos) {
        clientDriverLogger.debug(F("Found headers (buffer length: "), std::to_string(buff.length()), F(")"));
        for (const auto &key: this->http_->headersToCollect_) {
          if (buff.length() < key.length()) continue;
          std::string headerKey(buffer->begin(), 0, key.length());
          // Headers can't be UTF8 so we cool
          std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
            [](unsigned char c){ return std::tolower(c); });
          clientDriverLogger.debug(headerKey, F(" == "), key);
          if (headerKey != key.asCharPtr())
            continue;

          auto valueView = buff.substr(key.length() + 1); // ":"
          while (*valueView.begin() == ' ') valueView = valueView.substr(1);

          iop_assert(valueView.find("\r\n") != valueView.npos, F("Must contain endline"));
          std::string value(valueView, 0, valueView.find("\r\n"));
          clientDriverLogger.debug(F("Found header "), key, F(" = "), value, F("\n"));
          responseHeaders.emplace(std::string(key.asCharPtr()), value);

          iop_assert(buff.find("\r\n") > 0, F("Must contain endline"));
          const char* ptr = buff.begin() + buff.find("\r\n") + 2;
          memmove(buffer->data(), ptr, strlen(ptr) + 1);
          buff = buffer->begin();
          if (buff.find("\n") == buff.npos) iop_panic(F("Fuuuc")); //continue;
        }
        if (buff.find("\n") == buff.npos) {
          clientDriverLogger.warn(F("Newline missing in buffer: "), buff);
          return static_cast<int>(RawStatus::READ_FAILED);
        }
        clientDriverLogger.debug(F("Buffer: "), buff.substr(0, buff.find("\n") - 1));
        clientDriverLogger.debug(F("Skipping header ("), buff.substr(0, buff.find("\n") - 1), F(")"));
        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer->data(), ptr, strlen(ptr) + 1);
        buff = buffer->begin();
        if (buff.find("\n") == buff.npos) iop_panic(F("Fuuk")); //continue;
      } else {
        iop_panic(F("Press F to pay respect"));
      }
    }

    clientDriverLogger.debug(F("Payload ("), std::to_string(buff.length()), F(") ["), std::to_string(size), F("]: "), buff.substr(0, buff.find("\n") == buff.npos ? buff.find("\n") : buff.length()));

    responsePayload += buff;

    // TODO: For some weird reason `read` is blocking if we try to read after EOF
    // But it's not clear what will happen if EOF is exactly at buffer.size
    // Because it will continue, altough there isn't anything, but it's blocking on EOF
    // But this behavior is not documented. To replicate remove:
    // ` && len == buffer.size` and it will get stuck in the `read` above
    if (len > 0 && buff.length() > 0 && len == buffer->max_size())
      continue;
    break;
  }

  clientDriverLogger.debug(F("Close client: "), std::to_string(fd), F(" "), std::string(reinterpret_cast<const char*>(data), len));
  clientDriverLogger.info(F("Status: "), std::to_string(status.value_or(500)));
  iop_assert(status, F("Status not available"));
  return Response(responseHeaders, Payload(responsePayload), *status);
}

auto HTTPClient::begin(std::string uri_) noexcept -> std::optional<Session> {
  std::string_view uri(uri_);
    
  struct sockaddr_in serv_addr;
  int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(F("Unable to open socket"));
    return std::optional<Session>();
  }

  iop_assert(uri.find("http://") == 0, F("Protocol must be http (no SSL)"));
  uri = std::string_view(uri.begin() + 7);
  
  const auto portIndex = uri.find(iop::StaticString(F(":")).toString());
  uint16_t port = 443;
  if (portIndex != uri.npos) {
    auto end = uri.substr(portIndex + 1).find("/");
    if (end == uri.npos) end = uri.length();
    port = static_cast<uint16_t>(strtoul(std::string(uri.begin(), portIndex + 1, end).c_str(), nullptr, 10));
    if (port == 0) {
      clientDriverLogger.error(F("Unable to parse port, broken server: "), uri);
      return std::optional<Session>();
    }
  }
  clientDriverLogger.debug(F("Port: "), std::to_string(port));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
    close(fd);
    clientDriverLogger.error(F("Address not supported: "), host);
    return std::optional<Session>();
  }

  int32_t connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(F("Unnable to connect: "), std::to_string(connection));
    close(fd);
    return std::optional<Session>();
  }
  clientDriverLogger.debug(F("Began connection: "), uri);
  return Session(*this, std::move(uri_), fd);
}
}
#endif