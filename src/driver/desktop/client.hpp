#include "driver/client.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "driver/wifi.hpp"

#include <system_error>
#include <vector>
#include <string>
#include <unordered_map>

#include <algorithm>
#include <cctype>
#include <string>

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory>

#include <iostream>

static iop::Log clientDriverLogger(iop::LogLevel::DEBUG, IOP_STATIC_STRING("HTTP Client"));

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

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

HTTPClient::HTTPClient() noexcept: headersToCollect_() {}
HTTPClient::~HTTPClient() noexcept {}

static ssize_t send(uint32_t fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing())
    iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  return write(fd, msg, len);
}

Session::Session(HTTPClient &http, std::string uri, int32_t fd) noexcept: fd_(fd), http_(&http), headers{}, uri_(std::move(uri)) { IOP_TRACE(); }
Session::~Session() noexcept {
  if (this->fd_ != -1)
    close(this->fd_);
}
Session::Session(Session&& other) noexcept: fd_(other.fd_), http_(other.http_), headers(std::move(other.headers)), uri_(std::move(other.uri_)) {
  IOP_TRACE();
  other.http_ = nullptr;
  other.fd_ = -1;
}
auto Session::operator=(Session&& other) noexcept -> Session & {
  IOP_TRACE();
  this->http_ = other.http_;
  other.http_ = nullptr;

  this->uri_ = std::move(other.uri_);

  this->fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept {
  std::vector<std::string> vec;
  vec.reserve(count);
  for (size_t index = 0; index < count; ++index) {
    vec.push_back(headers[index]);
  }
  this->headersToCollect_ = std::move(vec);
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
  clientDriverLogger.debug(IOP_STATIC_STRING("Send request to "), path);

  auto fd = this->fd_;
  iop_assert(fd != -1, IOP_STATIC_STRING("Invalid file descriptor"));
  if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
    iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::START);
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
    iop::Log::print(IOP_STATIC_STRING("\n"), iop::LogLevel::TRACE, iop::LogType::END);
  clientDriverLogger.debug(IOP_STATIC_STRING("Sent data"));
  
  auto buffer = std::unique_ptr<char[]>(new (std::nothrow) char[4096]);
  iop_assert(buffer, IOP_STATIC_STRING("OOM"));
  
  ssize_t size = 0;
  auto firstLine = true;
  auto isPayload = false;
  
  auto status = std::make_optional(1000);
  std::string_view buff(buffer.get());
  while (true) {
    clientDriverLogger.debug(IOP_STATIC_STRING("Try read: "), buff);

    if (buff.length() < 4096 &&
        (size = read(fd, buffer.get() + buff.length(), 4096 - buff.length())) < 0) {
      clientDriverLogger.error(IOP_STATIC_STRING("Error reading from socket ("), std::to_string(size), IOP_STATIC_STRING("): "), std::to_string(errno), IOP_STATIC_STRING(" - "), strerror(errno)); 
      close(fd);
      return static_cast<int>(RawStatus::CONNECTION_FAILED);
    }
    buff = buffer.get();
    clientDriverLogger.debug(IOP_STATIC_STRING("Len: "), std::to_string(size));
    if (firstLine && size == 0) {
      close(fd);
      clientDriverLogger.warn(IOP_STATIC_STRING("Empty request: "), std::to_string(fd), IOP_STATIC_STRING(" "), std::string(reinterpret_cast<const char*>(data), len));
      return Response(status.value_or(500));
      //continue;
    }
    
    clientDriverLogger.debug(IOP_STATIC_STRING("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(IOP_STATIC_STRING("\n"))) continue;
    clientDriverLogger.debug(IOP_STATIC_STRING("Read: ("), std::to_string(size), IOP_STATIC_STRING(") ["), std::to_string(buff.length()), IOP_STATIC_STRING("]"));

    if (firstLine && buff.find("\n") == buff.npos) continue;

    if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
      clientDriverLogger.error(IOP_STATIC_STRING("Error reading first line: "), std::to_string(size));
      return static_cast<int>(RawStatus::READ_FAILED);
    }

    if (firstLine && size > 0) {
      clientDriverLogger.debug(IOP_STATIC_STRING("Found first line: "));

      const std::string_view statusStr(buffer.get() + 9); // len("HTTP/1.1 ") = 9
      const auto codeEnd = statusStr.find(" ");
      if (codeEnd == statusStr.npos) {
        clientDriverLogger.error(IOP_STATIC_STRING("Bad server: "), statusStr, IOP_STATIC_STRING(" -- "), buff);
        return static_cast<int>(RawStatus::READ_FAILED);
      }
      //iop_assert(buff.contains(IOP_STATIC_STRING("\n")), IOP_STATIC_STRING("First: ").toString() + std::to_string(strnlen(buffer.get(), 4096)) + IOP_STATIC_STRING(" bytes don't contain newline, the path is too long\n").toString());
      status = atoi(std::string(statusStr.begin(), 0, codeEnd).c_str());
      clientDriverLogger.debug(IOP_STATIC_STRING("Status: "), std::to_string(status.value_or(500)));
      firstLine = false;

      const char* ptr = buff.begin() + buff.find("\n") + 1;
      memmove(buffer.get(), ptr, strlen(ptr) + 1);
      buff = buffer.get();
    }
    if (!status) {
      clientDriverLogger.error(IOP_STATIC_STRING("No status"));
      return static_cast<int>(RawStatus::READ_FAILED);
    }
    clientDriverLogger.debug(IOP_STATIC_STRING("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(IOP_STATIC_STRING("\n"))) continue;
    //clientDriverLogger.debug(IOP_STATIC_STRING("Headers + Payload: "), buff.substr(0, buff.find("\n")));

    while (len > 0 && buff.length() > 0 && !isPayload) {
      // TODO: if empty line is split into t  wo reads (because of buff len) we are screwed
      //  || buff.contains(IOP_STATIC_STRING("\n\n")) || buff.contains(IOP_STATIC_STRING("\n\r\n"))
      if (buff.find("\r\n") == 0) {
        // TODO: move this logic to inside Response::await to be lazy-evaluated
        clientDriverLogger.debug(IOP_STATIC_STRING("Found Payload"));
        isPayload = true;

        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer.get(), ptr, strlen(ptr) + 1);
        buff = buffer.get();
        continue;
      } else if (buff.find("\r\n") == buff.npos) {
        iop_panic(IOP_STATIC_STRING("Bad software bruh"));
      } else if (buff.find("\r\n") != buff.npos) {
        clientDriverLogger.debug(IOP_STATIC_STRING("Found headers (buffer length: "), std::to_string(buff.length()), IOP_STATIC_STRING(")"));
        for (const auto &key: this->http_->headersToCollect_) {
          if (buff.length() < key.length()) continue;
          std::string headerKey(buffer.get(), 0, key.length());
          // Headers can't be UTF8 so we cool
          std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
            [](unsigned char c){ return std::tolower(c); });
          clientDriverLogger.debug(headerKey, IOP_STATIC_STRING(" == "), key);
          if (headerKey != key.c_str())
            continue;

          auto valueView = buff.substr(key.length() + 1); // ":"
          while (*valueView.begin() == ' ') valueView = valueView.substr(1);

          iop_assert(valueView.find("\r\n") != valueView.npos, IOP_STATIC_STRING("Must contain endline"));
          std::string value(valueView, 0, valueView.find("\r\n"));
          clientDriverLogger.debug(IOP_STATIC_STRING("Found header "), key, IOP_STATIC_STRING(" = "), value, IOP_STATIC_STRING("\n"));
          responseHeaders.emplace(std::string(key.c_str()), value);

          iop_assert(buff.find("\r\n") > 0, IOP_STATIC_STRING("Must contain endline"));
          const char* ptr = buff.begin() + buff.find("\r\n") + 2;
          memmove(buffer.get(), ptr, strlen(ptr) + 1);
          buff = buffer.get();
          if (buff.find("\n") == buff.npos) iop_panic(IOP_STATIC_STRING("Fuuuc")); //continue;
        }
        if (buff.find("\n") == buff.npos) {
          clientDriverLogger.warn(IOP_STATIC_STRING("Newline missing in buffer: "), buff);
          return static_cast<int>(RawStatus::READ_FAILED);
        }
        clientDriverLogger.debug(IOP_STATIC_STRING("Buffer: "), buff.substr(0, buff.find("\n") - 1));
        clientDriverLogger.debug(IOP_STATIC_STRING("Skipping header ("), buff.substr(0, buff.find("\n") - 1), IOP_STATIC_STRING(")"));
        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer.get(), ptr, strlen(ptr) + 1);
        buff = buffer.get();
        if (buff.find("\n") == buff.npos) iop_panic(IOP_STATIC_STRING("Fuuk")); //continue;
      } else {
        iop_panic(IOP_STATIC_STRING("Press F to pay respect"));
      }
    }

    clientDriverLogger.debug(IOP_STATIC_STRING("Payload ("), std::to_string(buff.length()), IOP_STATIC_STRING(") ["), std::to_string(size), IOP_STATIC_STRING("]: "), buff.substr(0, buff.find("\n") == buff.npos ? buff.find("\n") : buff.length()));

    responsePayload += buff;

    // TODO: For some weird reason `read` is blocking if we try to read after EOF
    // But it's not clear what will happen if EOF is exactly at buffer.size
    // Because it will continue, altough there isn't anything, but it's blocking on EOF
    // But this behavior is not documented. To replicate remove:
    // ` && len == buffer.size` and it will get stuck in the `read` above
    if (len > 0 && buff.length() > 0 && len == 4096)
      continue;
    break;
  }

  clientDriverLogger.debug(IOP_STATIC_STRING("Close client: "), std::to_string(fd), IOP_STATIC_STRING(" "), std::string(reinterpret_cast<const char*>(data), len));
  clientDriverLogger.info(IOP_STATIC_STRING("Status: "), std::to_string(status.value_or(500)));
  iop_assert(status, IOP_STATIC_STRING("Status not available"));
  return Response(responseHeaders, Payload(responsePayload), *status);
}

auto HTTPClient::begin(std::string uri_) noexcept -> std::optional<Session> {
  std::string_view uri(uri_);
    
  struct sockaddr_in serv_addr;
  int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(IOP_STATIC_STRING("Unable to open socket"));
    return std::optional<Session>();
  }

  iop_assert(uri.find("http://") == 0, IOP_STATIC_STRING("Protocol must be http (no SSL)"));
  uri = std::string_view(uri.begin() + 7);
  
  const auto portIndex = uri.find(IOP_STATIC_STRING(":").toString());
  uint16_t port = 443;
  if (portIndex != uri.npos) {
    auto end = uri.substr(portIndex + 1).find("/");
    if (end == uri.npos) end = uri.length();
    port = static_cast<uint16_t>(strtoul(std::string(uri.begin(), portIndex + 1, end).c_str(), nullptr, 10));
    if (port == 0) {
      clientDriverLogger.error(IOP_STATIC_STRING("Unable to parse port, broken server: "), uri);
      return std::optional<Session>();
    }
  }
  clientDriverLogger.debug(IOP_STATIC_STRING("Port: "), std::to_string(port));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
    close(fd);
    clientDriverLogger.error(IOP_STATIC_STRING("Address not supported: "), host);
    return std::optional<Session>();
  }

  int32_t connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(IOP_STATIC_STRING("Unable to connect: "), std::to_string(connection));
    close(fd);
    return std::optional<Session>();
  }
  clientDriverLogger.debug(IOP_STATIC_STRING("Began connection: "), uri);
  return Session(*this, std::move(uri_), fd);
}
}