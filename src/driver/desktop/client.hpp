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

static iop::Log clientDriverLogger(iop::LogLevel::DEBUG, FLASH("HTTP Client"));

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
  clientDriverLogger.debug(FLASH("Send request to "), path);

  auto fd = this->fd_;
  iop_assert(fd != -1, FLASH("Invalid file descriptor"));
  if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
    iop::Log::print(FLASH(""), iop::LogLevel::TRACE, iop::LogType::START);
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
    iop::Log::print(FLASH("\n"), iop::LogLevel::TRACE, iop::LogType::END);
  clientDriverLogger.debug(FLASH("Sent data"));
  
  auto buffer = std::unique_ptr<std::array<char, 4096>>(new (std::nothrow) std::array<char, 4096>>());
  iop_assert(buffer, FLASH("OOM"));
  
  ssize_t size = 0;
  auto firstLine = true;
  auto isPayload = false;
  
  auto status = std::make_optional(1000);
  std::string_view buff(buffer->begin());
  while (true) {
    clientDriverLogger.debug(FLASH("Try read: "), buff);

    if (buff.length() < buffer->max_size() &&
        (size = read(fd, buffer->data() + buff.length(), buffer->max_size() - buff.length())) < 0) {
      clientDriverLogger.error(FLASH("Error reading from socket ("), std::to_string(size), FLASH("): "), std::to_string(errno), FLASH(" - "), strerror(errno)); 
      close(fd);
      return static_cast<int>(RawStatus::CONNECTION_FAILED);
    }
    buff = buffer->begin();
    clientDriverLogger.debug(FLASH("Len: "), std::to_string(size));
    if (firstLine && size == 0) {
      close(fd);
      clientDriverLogger.warn(FLASH("Empty request: "), std::to_string(fd), FLASH(" "), std::string(reinterpret_cast<const char*>(data), len));
      return Response(status.value_or(500));
      //continue;
    }
    
    clientDriverLogger.debug(FLASH("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(FLASH("\n"))) continue;
    clientDriverLogger.debug(FLASH("Read: ("), std::to_string(size), FLASH(") ["), std::to_string(buff.length()), FLASH("]"));

    if (firstLine && buff.find("\n") == buff.npos) continue;

    if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
      clientDriverLogger.error(FLASH("Error reading first line: "), std::to_string(size));
      return static_cast<int>(RawStatus::READ_FAILED);
    }

    if (firstLine && size > 0) {
      clientDriverLogger.debug(FLASH("Found first line: "));

      const std::string_view statusStr(buffer->begin() + 9); // len("HTTP/1.1 ") = 9
      const auto codeEnd = statusStr.find(" ");
      if (codeEnd == statusStr.npos) {
        clientDriverLogger.error(FLASH("Bad server: "), statusStr, FLASH(" -- "), buff);
        return static_cast<int>(RawStatus::READ_FAILED);
      }
      //iop_assert(buff.contains(FLASH("\n")), FLASH("First: ").toString() + std::to_string(buffer.length()) + FLASH(" bytes don't contain newline, the path is too long\n").toString());
      status = atoi(std::string(statusStr.begin(), 0, codeEnd).c_str());
      clientDriverLogger.debug(FLASH("Status: "), std::to_string(status.value_or(500)));
      firstLine = false;

      const char* ptr = buff.begin() + buff.find("\n") + 1;
      memmove(buffer->data(), ptr, strlen(ptr) + 1);
      buff = buffer->begin();
    }
    if (!status) {
      clientDriverLogger.error(FLASH("No status"));
      return static_cast<int>(RawStatus::READ_FAILED);
    }
    clientDriverLogger.debug(FLASH("Buffer: "), buff.substr(0, buff.find("\n") - 1));
    //if (!buff.contains(FLASH("\n"))) continue;
    //clientDriverLogger.debug(FLASH("Headers + Payload: "), buff.substr(0, buff.find("\n")));

    while (len > 0 && buff.length() > 0 && !isPayload) {
      // TODO: if empty line is split into t  wo reads (because of buff len) we are screwed
      //  || buff.contains(FLASH("\n\n")) || buff.contains(FLASH("\n\r\n"))
      if (buff.find("\r\n") == 0) {
        // TODO: move this logic to inside Response::await to be lazy-evaluated
        clientDriverLogger.debug(FLASH("Found Payload"));
        isPayload = true;

        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer->data(), ptr, strlen(ptr) + 1);
        buff = buffer->begin();
        continue;
      } else if (buff.find("\r\n") == buff.npos) {
        iop_panic(FLASH("Bad software bruh"));
      } else if (buff.find("\r\n") != buff.npos) {
        clientDriverLogger.debug(FLASH("Found headers (buffer length: "), std::to_string(buff.length()), FLASH(")"));
        for (const auto &key: this->http_->headersToCollect_) {
          if (buff.length() < key.length()) continue;
          std::string headerKey(buffer->begin(), 0, key.length());
          // Headers can't be UTF8 so we cool
          std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
            [](unsigned char c){ return std::tolower(c); });
          clientDriverLogger.debug(headerKey, FLASH(" == "), key);
          if (headerKey != key.asCharPtr())
            continue;

          auto valueView = buff.substr(key.length() + 1); // ":"
          while (*valueView.begin() == ' ') valueView = valueView.substr(1);

          iop_assert(valueView.find("\r\n") != valueView.npos, FLASH("Must contain endline"));
          std::string value(valueView, 0, valueView.find("\r\n"));
          clientDriverLogger.debug(FLASH("Found header "), key, FLASH(" = "), value, FLASH("\n"));
          responseHeaders.emplace(std::string(key.asCharPtr()), value);

          iop_assert(buff.find("\r\n") > 0, FLASH("Must contain endline"));
          const char* ptr = buff.begin() + buff.find("\r\n") + 2;
          memmove(buffer->data(), ptr, strlen(ptr) + 1);
          buff = buffer->begin();
          if (buff.find("\n") == buff.npos) iop_panic(FLASH("Fuuuc")); //continue;
        }
        if (buff.find("\n") == buff.npos) {
          clientDriverLogger.warn(FLASH("Newline missing in buffer: "), buff);
          return static_cast<int>(RawStatus::READ_FAILED);
        }
        clientDriverLogger.debug(FLASH("Buffer: "), buff.substr(0, buff.find("\n") - 1));
        clientDriverLogger.debug(FLASH("Skipping header ("), buff.substr(0, buff.find("\n") - 1), FLASH(")"));
        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer->data(), ptr, strlen(ptr) + 1);
        buff = buffer->begin();
        if (buff.find("\n") == buff.npos) iop_panic(FLASH("Fuuk")); //continue;
      } else {
        iop_panic(FLASH("Press F to pay respect"));
      }
    }

    clientDriverLogger.debug(FLASH("Payload ("), std::to_string(buff.length()), FLASH(") ["), std::to_string(size), FLASH("]: "), buff.substr(0, buff.find("\n") == buff.npos ? buff.find("\n") : buff.length()));

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

  clientDriverLogger.debug(FLASH("Close client: "), std::to_string(fd), FLASH(" "), std::string(reinterpret_cast<const char*>(data), len));
  clientDriverLogger.info(FLASH("Status: "), std::to_string(status.value_or(500)));
  iop_assert(status, FLASH("Status not available"));
  return Response(responseHeaders, Payload(responsePayload), *status);
}

auto HTTPClient::begin(std::string uri_) noexcept -> std::optional<Session> {
  std::string_view uri(uri_);
    
  struct sockaddr_in serv_addr;
  int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    clientDriverLogger.error(FLASH("Unable to open socket"));
    return std::optional<Session>();
  }

  iop_assert(uri.find("http://") == 0, FLASH("Protocol must be http (no SSL)"));
  uri = std::string_view(uri.begin() + 7);
  
  const auto portIndex = uri.find(FLASH(":").toString());
  uint16_t port = 443;
  if (portIndex != uri.npos) {
    auto end = uri.substr(portIndex + 1).find("/");
    if (end == uri.npos) end = uri.length();
    port = static_cast<uint16_t>(strtoul(std::string(uri.begin(), portIndex + 1, end).c_str(), nullptr, 10));
    if (port == 0) {
      clientDriverLogger.error(FLASH("Unable to parse port, broken server: "), uri);
      return std::optional<Session>();
    }
  }
  clientDriverLogger.debug(FLASH("Port: "), std::to_string(port));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  auto end = uri.find(":");
  if (end == uri.npos) end = uri.find("/");
  if (end == uri.npos) end = uri.length();
  
  const auto host = std::string(uri.begin(), 0, end);
  // Convert IPv4 and IPv6 addresses from text to binary form
  if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
    close(fd);
    clientDriverLogger.error(FLASH("Address not supported: "), host);
    return std::optional<Session>();
  }

  int32_t connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  if (connection < 0) {
    clientDriverLogger.error(FLASH("Unnable to connect: "), std::to_string(connection));
    close(fd);
    return std::optional<Session>();
  }
  clientDriverLogger.debug(FLASH("Began connection: "), uri);
  return Session(*this, std::move(uri_), fd);
}
}