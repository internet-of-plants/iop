#include "core/utils.hpp"

#ifndef IOP_ONLINE
#define IOP_DRIVER_CLIENT
#endif

#ifndef IOP_DRIVER_CLIENT
#define IOP_DRIVER_CLIENT

#define HTTPC_ERROR_CONNECTION_FAILED   (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)

#include <vector>
#include <string>
#include <unordered_map>
#include "core/string/static.hpp"
#include "core/string/fixed.hpp"
#include "core/log.hpp"

#include <algorithm>
#include <cctype>
#include <string>

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

const static iop::Log clientDriverLogger(iop::LogLevel::WARN, F("HTTP Client"));

static int send__(uint32_t fd, const char * msg, const size_t len) noexcept {
  if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
    iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::CONTINUITY);
  return write(fd, msg, len);
}
static int recv(uint32_t fd, char *msg, size_t len) {
  return read(fd, msg, len);
}

typedef enum {
    HTTP_CODE_CONTINUE = 100,
    HTTP_CODE_SWITCHING_PROTOCOLS = 101,
    HTTP_CODE_PROCESSING = 102,
    HTTP_CODE_OK = 200,
    HTTP_CODE_CREATED = 201,
    HTTP_CODE_ACCEPTED = 202,
    HTTP_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_CODE_NO_CONTENT = 204,
    HTTP_CODE_RESET_CONTENT = 205,
    HTTP_CODE_PARTIAL_CONTENT = 206,
    HTTP_CODE_MULTI_STATUS = 207,
    HTTP_CODE_ALREADY_REPORTED = 208,
    HTTP_CODE_IM_USED = 226,
    HTTP_CODE_MULTIPLE_CHOICES = 300,
    HTTP_CODE_MOVED_PERMANENTLY = 301,
    HTTP_CODE_FOUND = 302,
    HTTP_CODE_SEE_OTHER = 303,
    HTTP_CODE_NOT_MODIFIED = 304,
    HTTP_CODE_USE_PROXY = 305,
    HTTP_CODE_TEMPORARY_REDIRECT = 307,
    HTTP_CODE_PERMANENT_REDIRECT = 308,
    HTTP_CODE_BAD_REQUEST = 400,
    HTTP_CODE_UNAUTHORIZED = 401,
    HTTP_CODE_PAYMENT_REQUIRED = 402,
    HTTP_CODE_FORBIDDEN = 403,
    HTTP_CODE_NOT_FOUND = 404,
    HTTP_CODE_METHOD_NOT_ALLOWED = 405,
    HTTP_CODE_NOT_ACCEPTABLE = 406,
    HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
    HTTP_CODE_REQUEST_TIMEOUT = 408,
    HTTP_CODE_CONFLICT = 409,
    HTTP_CODE_GONE = 410,
    HTTP_CODE_LENGTH_REQUIRED = 411,
    HTTP_CODE_PRECONDITION_FAILED = 412,
    HTTP_CODE_PAYLOAD_TOO_LARGE = 413,
    HTTP_CODE_URI_TOO_LONG = 414,
    HTTP_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
    HTTP_CODE_RANGE_NOT_SATISFIABLE = 416,
    HTTP_CODE_EXPECTATION_FAILED = 417,
    HTTP_CODE_MISDIRECTED_REQUEST = 421,
    HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
    HTTP_CODE_LOCKED = 423,
    HTTP_CODE_FAILED_DEPENDENCY = 424,
    HTTP_CODE_UPGRADE_REQUIRED = 426,
    HTTP_CODE_PRECONDITION_REQUIRED = 428,
    HTTP_CODE_TOO_MANY_REQUESTS = 429,
    HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
    HTTP_CODE_NOT_IMPLEMENTED = 501,
    HTTP_CODE_BAD_GATEWAY = 502,
    HTTP_CODE_SERVICE_UNAVAILABLE = 503,
    HTTP_CODE_GATEWAY_TIMEOUT = 504,
    HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
    HTTP_CODE_VARIANT_ALSO_NEGOTIATES = 506,
    HTTP_CODE_INSUFFICIENT_STORAGE = 507,
    HTTP_CODE_LOOP_DETECTED = 508,
    HTTP_CODE_NOT_EXTENDED = 510,
    HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511
} t_http_codes;

class Stream {
  uint32_t fd;
public:
  Stream(uint32_t fd): fd(fd) {}

  int write(const char *msg, size_t len) {
    return send__(this->fd, msg, len);
  }
  int read(char *buff, size_t len) {
    return recv(this->fd, buff, len);
  }
};

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

  std::optional<int32_t> currentFd;
public:
  void setReuse(bool reuse) {}
  void collectHeaders(const char **headerKeys, size_t count) {
    for (uint8_t index = 0; index < count; ++index) {
        this->headersToCollect.push_back(headerKeys[index]);
    }
  }
  std::string header(std::string key) {
    if (this->responseHeaders.count(key) == 0) return "";
    return this->responseHeaders.at(key);
  }
  size_t getSize() {
    return this->responsePayload.length();
  }
  std::string getString() {
    return this->responsePayload;
  }
  void end() {
    if (this->currentFd.has_value())
      close(iop::unwrap(this->currentFd, IOP_CTX()));

    this->authorization.clear();
    this->responsePayload.clear();
    this->responseHeaders.clear();
    this->uri.clear();
  }
  void addHeader(iop::StaticString key, iop::StaticString value) {
    auto keyLower = key.toStdString();
    // Headers can't be UTF8 so we cool
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
        [](unsigned char c){ return std::tolower(c); });
    this->headers.emplace(keyLower, std::move(value.toStdString()));
  }
  void addHeader(iop::StaticString key, std::string value) {
    auto keyLower = key.toStdString();
    // Headers can't be UTF8 so we cool
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
        [](unsigned char c){ return std::tolower(c); });
    this->headers.emplace(keyLower, value);
  }
  void setTimeout(uint32_t ms) {}
  void setAuthorization(std::string auth) {
    if (auth.length() == 0) return;
    this->headers.emplace(std::string("Authorization"), std::string("Basic ") + auth);
  }
  int sendRequest(std::string method, const uint8_t *data, size_t len) {
    this->responsePayload.clear();

    const std::string_view path(this->uri.c_str() + this->uri.find("/", this->uri.find("://") + 3));
    clientDriverLogger.debug(F("Send request to "), std::string(path));

    uint32_t fd = iop::unwrap_ref(this->currentFd, IOP_CTX());
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::START);
    send__(fd, method.c_str(), method.length());
    send__(fd, " ", 1);
    send__(fd, path.begin(), path.length());
    send__(fd, " HTTP/1.0\r\n", 11);
    send__(fd, "Content-Length: ", 16);
    const auto dataLengthStr = std::to_string(len);
    send__(fd, dataLengthStr.c_str(), dataLengthStr.length());
    send__(fd, "\r\n", 2);
    for (const auto& [key, value]: this->headers) {
      send__(fd, key.c_str(), key.length());
      send__(fd, ": ", 2);
      send__(fd, value.c_str(), value.length());
      send__(fd, "\r\n", 2);
    }
    send__(fd, "\r\n", 2);
    send__(fd, (char*)data, len);
    if (clientDriverLogger.level() == iop::LogLevel::TRACE || iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::END);
    clientDriverLogger.debug(F("Sent data"));
    
    static auto buffer = iop::FixedString<1024>::empty();
    buffer.clear();
    
    int32_t size = 0;
    auto firstLine = true;
    auto isPayload = false;
    
    auto status = std::make_optional(500);
    auto *buffStart = buffer.asMut();
    iop::StringView buff(buffer);
    while (true) {
      clientDriverLogger.debug(F("Try read: "), std::to_string(buffer.length()));

      buffStart += buffer.length();
      if (buffer.length() < buffer.size &&
          (size = read(fd, buffStart, buffer.size - buffer.length())) < 0) {
        clientDriverLogger.error(F("Error reading from socket ("), std::to_string(size), F("): "), std::to_string(errno), F(" - "), iop::UnsafeRawString(strerror(errno))); 
        close(fd);
        return 500;
      }
      clientDriverLogger.debug(F("Len: "), std::to_string(size));
      if (firstLine && size == 0) {
        clientDriverLogger.error(F("Empty request"));
        close(fd);
        return 500;
      }
      
      iop_assert(buff.contains(F("\n")), F("We need a better HTTP client"));
      clientDriverLogger.debug(F("Read: ("), std::to_string(size), F(") ["), std::to_string(buffer.length()), F("]: "), std::string(buffer.get()).substr(0, buff.indexOf(F("\n"))));

      if (firstLine && size < 10) { // len("HTTP/1.1 ") = 9
        clientDriverLogger.error(F("Error reading first line: "), std::to_string(size));
        return 500;
      }

      if (firstLine && size > 0) {
        clientDriverLogger.debug(F("Found first line: "));

        const iop::StringView statusStr(iop::UnsafeRawString(buffer.get() + 9)); // len("HTTP/1.1 ") = 9
        const auto codeEnd = statusStr.indexOf(F(" "));
        if (codeEnd == -1) {
          clientDriverLogger.error(F("Bad server: "), statusStr, F(" -- "), buffer);
          return 500;
        }
        iop_assert(buff.contains(F("\n")), iop::StaticString(F("First: ")).toStdString() + std::to_string(buffer.length()) + iop::StaticString(F(" bytes don't contain newline, the path is too long\n")).toStdString());
        status = std::make_optional(atoi(std::string(statusStr.get(), 0, codeEnd).c_str()));
        clientDriverLogger.debug(F("Status: "), std::to_string(status.value_or(500)));
        firstLine = false;

        const char* ptr = buff.get() + buff.indexOf(F("\n")) + 1;
        memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
      }
      if (!status.has_value()) {
        clientDriverLogger.error(F("No status"));
        return 500;
      }
      iop_assert(buff.contains(F("\n")), F("We need a better HTTP client"));
      clientDriverLogger.debug(F("Headers + Payload: "), std::string(buffer.get()).substr(0, buff.indexOf(F("\n"))));

      while (len > 0 && buffer.length() > 0 && !isPayload) {
        // TODO: if empty line is split into t  wo reads (because of buff len) we are screwed
        //  || buff.contains(F("\n\n")) || buff.contains(F("\n\r\n"))
        if (buff.indexOf(F("\r\n")) == 0) {
          clientDriverLogger.debug(F("Found Payload"));
          isPayload = true;

          const char* ptr = buff.get() + buff.indexOf(F("\r\n")) + 2;
          memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
          continue;
        } else if (!buff.contains(F("\r\n"))) {
          iop_panic(F("Bad software bruh"));
        } else if (buff.contains(F("\r\n"))) {
          clientDriverLogger.debug(F("Found headers (buffer length: "), std::to_string(buff.length()), F(")"));
          auto found = false;
          for (const auto &key: this->headersToCollect) {
            if (buff.length() < key.length()) continue;
            std::string headerKey(buffer.get(), 0, key.length());
            // Headers can't be UTF8 so we cool
            std::transform(headerKey.begin(), headerKey.end(), headerKey.begin(),
              [](unsigned char c){ return std::tolower(c); });
            clientDriverLogger.debug(headerKey, F(" == "), key);
            if (headerKey != key)
              continue;

            found = true;
            iop::StringView valueView(buff.get() + key.length() + 1); // ":"
            if (*valueView.get() == ' ') valueView = iop::StringView(valueView.get() + 1);

            iop_assert(valueView.contains(F("\r\n")), F("Must contain endline"));
            const std::string value(valueView.get(), 0, valueView.indexOf(F("\r\n")));
            clientDriverLogger.debug(F("Found header "), key, F(" = "), value, F("\n"));
            this->responseHeaders.emplace(key, value);

            iop_assert(buff.contains(F("\r\n")), F("Must contain endline"));
            const char* ptr = buff.get() + buff.indexOf(F("\r\n")) + 2;
            memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
            if (!buff.contains(F("\n"))) iop_panic(F("Fuuuc")); //continue;
          }
          if (!buff.contains(F("\n"))) continue;
          clientDriverLogger.debug(F("Buffer: "), std::string(buffer.get()).substr(0, buff.indexOf(F("\n"))));
          clientDriverLogger.debug(F("Skipping header ("), std::to_string(buff.indexOf(F("\r\n"))), F(")"));
          const char* ptr = buff.get() + buff.indexOf(F("\r\n")) + 2;
          memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
          if (!buff.contains(F("\n"))) iop_panic(F("Fuuk")); //continue;
        } else {
          iop_panic(F("Press F to pay respect"));
        }
      }

      clientDriverLogger.debug(F("Payload ("), std::to_string(buff.length()), F(") ["), std::to_string(size), F("]: "), std::string(buffer.get()).substr(0, buff.contains(F("\n")) ? buff.indexOf(F("\n")) : buffer.length()));

      this->responsePayload += buff.get();

      // TODO: For some weird reason `read` is blocking if we try to read after EOF
      // But it's not clear what will happen if EOF is exactly at buffer.size
      // Because it will continue, altough there isn't anything, but it's blocking on EOF
      // But this behavior is not documented. To replicate remove:
      // ` && len == buffer.size` and it will get stuck in the `read` above
      if (len > 0 && buff.length() > 0 && len == buffer.size)
        continue;
      break;
    }

    clientDriverLogger.debug(F("Close client"));
    close(fd);
    return iop::unwrap(status, IOP_CTX());
  }

  bool begin(WiFiClient client, std::string host, uint32_t port, std::string uri) {
    return this->begin(client, std::string("http://") + host + ":" + std::to_string(port) + uri);
  }

  bool begin(WiFiClient client, std::string uri_) {
    this->end();
    this->uri = uri_;
    (void) client;

    iop::StringView uri(uri_);
     
    struct sockaddr_in serv_addr;
    int32_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
      clientDriverLogger.error(F("Unable to open socket"));
      return false;
    }

    iop_assert(uri.indexOf(F("http://")) == 0, F("Protocol must be http (no SSL)"));
    uri = iop::StringView(iop::UnsafeRawString(uri.get() + 7));
    
    const auto portIndex = uri.indexOf(F(":"));
    uint16_t port = 443;
    if (portIndex != -1) {
      auto end = iop::StringView(iop::UnsafeRawString(uri.get() + portIndex + 1)).indexOf(F("/"));
      if (end == -1) end = uri.length();
      port = atoi(std::string(uri.get(), portIndex + 1, end).c_str());
    }
    clientDriverLogger.debug(F("Port: "), std::to_string(port));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    auto end = uri.indexOf(F(":"));
    if (end == -1) end = uri.indexOf(F("/"));
    if (end == -1) end = uri.length();
    
    const auto host = std::string(uri.get(), 0, end);
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0) {
      close(fd);
      clientDriverLogger.error(F("Address not supported: "), host);
      return false;
    }

    int32_t connection = connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (connection < 0) {
      clientDriverLogger.error(F("Unnable to connect: "), std::to_string(connection));
      close(fd);
      return false;
    }
    clientDriverLogger.debug(F("Began connection: "), uri);
    this->currentFd = std::make_optional(fd);
    return true;
  }
};

#endif