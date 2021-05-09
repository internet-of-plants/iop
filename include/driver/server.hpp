#ifndef IOP_DRIVER_SERVER
#define IOP_DRIVER_SERVER

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "core/string/fixed.hpp"
#include <optional>

#include <iostream>

// TODO: Make it multiplatform
// Berkeley sockets, so assumes POSIX compliant OS //
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <chrono>

void stop(uint32_t fd) {
  close(fd);
}

namespace iop {
std::string httpCodeToString(const int code) {
  if (code == 200) {
    return "OK";
  } else if (code == 302) {
    return "Found";
  } else if (code == 404) {
    return "Not Found";
  } else {
    iop_panic(std::string("Http code not known: ") + std::to_string(code));
  }
}
}

enum class HTTPMethod { ANY, GET, HEAD, POST, PUT, PATCH, DELETE, OPTIONS };
enum class HTTPUploadStatus { START, WRITE, END, ABORTED };
enum class HTTPAuthMethod { BASIC, DIGEST };

#define HTTP_DOWNLOAD_UNIT_SIZE 1460

#define HTTP_MAX_DATA_WAIT 5000 //ms to wait for the client to send the request
#define HTTP_MAX_POST_WAIT 5000 //ms to wait for POST data to arrive
#define HTTP_MAX_SEND_WAIT 5000 //ms to wait for data chunk to be ACKed
#define HTTP_MAX_CLOSE_WAIT 2000 //ms to wait for the client to close the connection

#define CONTENT_LENGTH_UNKNOWN ((size_t) -1)
#define CONTENT_LENGTH_NOT_SET ((size_t) -2)

// THIS IS NOT THREAD SAFE
class ESP8266WebServer {
  using Buffer = iop::FixedString<1024>;

  std::unordered_map<std::string, std::function<void(void)>> router;
  std::function<void(void)> notFoundHandler;
  Buffer buffer;
  uint16_t port;

  std::optional<int32_t> maybeFd;
  std::optional<struct sockaddr_in> maybeAddress;

  std::optional<int32_t> currentClient;
  std::string currentHeaders;
  std::string currentPayload;
  std::optional<size_t> currentContentLength;
  std::string currentRoute;

public:
  ESP8266WebServer(uint16_t port = 8082): port(port), buffer(Buffer::empty()) {
    this->notFoundHandler = [this]() {
      this->send_P(404, "text/plain", "Not Found");
    };
  }
  ~ESP8266WebServer() {}

  enum class ClientFuture { REQUEST_CAN_CONTINUE, REQUEST_IS_HANDLED, MUST_STOP, IS_GIVEN };

  void begin() {
    this->close();

    int32_t fd = 0;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        std::cout << "Unable to open socket" << std::endl;
        return;
    }
    this->maybeFd = std::make_optional(fd);

    // Posix boilerplate
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(this->port);
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));

    if (bind(fd, (struct sockaddr* )&address, sizeof(address)) < 0) {
        std::cout << "Unable to bind socket (" << errno << "): " << strerror(errno) << std::endl;
        return;
    }

    if (listen(fd, 100) < 0) {
        std::cout << "Unable to listen socket" << std::endl;
        return;
    }

    this->maybeAddress = std::make_optional(address);
  }
  
  void handleClient() {
    this->endRequest();

    if (!this->maybeFd.has_value()) {
      std::cout << "Fd is not found" << std::endl;
      return;
    }
    int32_t fd = this->maybeFd.value();

    if (!this->maybeAddress.has_value()) {
      return;
    }
    struct sockaddr_in address = this->maybeAddress.value();
    socklen_t addr_len = sizeof(address);

    int32_t client = 0;
    if ((client = accept4(fd, (struct sockaddr *)&address, &addr_len, O_NONBLOCK)) <= 0) {
      if (client == 0) return;

      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        std::cout << "Error accepting connection (" << errno << "): " << strerror(errno) << std::endl;
      }
      return;
    }
    std::cout << "Accepted connection" << std::endl;
    this->currentClient = std::make_optional(client);

    bool firstLine = true;
    bool isPayload = false;
    
    this->buffer.clear();
    while (true) {
      ssize_t len = 0;
      if ((len = read(client, this->buffer.asMut(), this->buffer.size - this->buffer.length())) <= 0) {
        if (len != 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(50ms);
          } else {
            std::cout << "Error reading from socket (" << errno << "): " << strerror(errno) << std::endl;
            return;
          }
        }
      }
      std::cout << "Len: " << len << std::endl;
      if (!isPayload && len < 0) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
        continue;
      }
      if (firstLine == true && len == 0) return;
      std::cout << "Read: (" << len << ") [" << this->buffer.length() << "]: " << this->buffer.get() << std::endl;

      iop::StringView buff(this->buffer);
      if (len > 0 && firstLine) {
        if (buff.contains(F("POST"))) {
          const ssize_t space = iop::StringView(iop::UnsafeRawString(buff.get() + 5)).indexOf(F(" "));
          this->currentRoute = std::string(buff.get() + 5, space);
          std::cout << "POST: " << this->currentRoute << std::endl;
        } else if (buff.contains(F("GET"))) {
          const ssize_t space = iop::StringView(iop::UnsafeRawString(buff.get() + 4)).indexOf(F(" "));
          this->currentRoute = std::string(buff.get() + 4, space);
          std::cout << "GET: " << this->currentRoute << std::endl;
        } else {
          iop_panic(iop::StaticString(F("Unimplemented HTTP method: ")).toStdString() + buff.get());
        }
        firstLine = false;
        
        iop_assert(buff.contains(F("\n")), F("Buffer doesn't contain \n"));
        std::cout << "Found first line" << std::endl;
        const char* ptr = this->buffer.get() + buff.indexOf(F("\n")) + 1;
        memmove(this->buffer.asMut(), ptr, strlen(ptr) + 1);
        if (!buff.contains(F("\n"))) continue;
      }
      iop_assert(this->currentRoute.length() > 0, F("Route is None"));
      std::cout << "Headers + Payload: " << this->buffer.get() << std::endl;

      while (len > 0 && this->buffer.length() > 0 && !isPayload) {
        // TODO: if empty line is split into two reads (because of buff len) we are screwed
        //  || buff.contains(F("\n\n")) || buff.contains(F("\n\r\n"))
        if (buff.contains(F("\r\n")) && buff.indexOf(F("\r\n")) == buff.indexOf(F("\r\n\r\n"))) {
          isPayload = true;

          const char* ptr = this->buffer.get() + buff.indexOf(F("\r\n\r\n")) + 4;
          memmove(this->buffer.asMut(), ptr, strlen(ptr) + 1);
          if (!buff.contains(F("\n"))) continue;
        } else if (!buff.contains(F("\r\n"))) {
          iop_panic(F("Bad code"));
        } else {
          const char* ptr = this->buffer.get() + buff.indexOf(F("\r\n")) + 2;
          memmove(this->buffer.asMut(), ptr, strlen(ptr) + 1);
          // TODO: could this enter in a infinite loop?
          if (!buff.contains(F("\n"))) continue;
        }
      }

      std::cout << "Payload (" << this->buffer.length() << ") [" << len << "]: " << this->buffer.get() << std::endl;

      if (len > 0) {
        this->currentPayload += buff.get();
        continue;
      }

      std::cout << "Route: " << this->currentRoute << std::endl;
      if (this->router.count(this->currentRoute) != 0) {
        this->router.at(this->currentRoute)();
      } else {
        std::cout << "Route not found" << std::endl;
        this->notFoundHandler();
      }
      break;
    }

    std::cout << "Close client" << std::endl;
    this->endRequest();
  }

  void endRequest() {
    this->currentHeaders = "";
    this->currentPayload = "";
    this->currentContentLength.reset();
    this->buffer.clear();
    if (this->currentClient.has_value()) {
      stop(this->currentClient.value());
      this->currentClient.reset();
    }
  }

  void close() {
    this->maybeAddress.reset();
    this->endRequest();

    if (this->maybeFd.has_value()) {
      stop(this->maybeFd.value());
      this->maybeFd.reset();
    }
  }

  void on(iop::StaticString uri, std::function<void(void)> handler) {
    this->router.emplace(std::move(uri.toStdString()), std::move(handler));
  }
  //called when handler is not assigned
  void onNotFound(std::function<void(void)> fn) {
    this->notFoundHandler = fn;
  }
  
  // get request argument value by name
  std::string arg(iop::StaticString name) const {
    iop::StringView view(this->currentPayload);
    const size_t argEncodingLen = 2 + name.length(); // len('&') + len('=') + len(name)
    ssize_t index = view.indexOf(std::string("&") + name.asCharPtr() + "=");
    if (index < 0) index = view.indexOf(std::string("?") + name.asCharPtr() + "=");
    if (index < 0) return "";
    const ssize_t end = iop::StringView(iop::UnsafeRawString(view.get() + index + argEncodingLen)).indexOf(F("&"));
    if (end < 0) return view.get() + index + argEncodingLen;
    return std::string(view.get() + index + argEncodingLen, end);
  }
  // check if argument exists
  bool hasArg(iop::StaticString name) const {
    iop::StringView view(this->currentPayload);
    return view.contains(std::string("&") + name.asCharPtr() + "=")
      || view.contains(std::string("?") + name.asCharPtr() + "=");
  }

  // send response to the client
  // code - HTTP response code, can be 200 or 404
  // content_type - HTTP content type, like "text/plain" or "image/png"
  // content - actual content body
  
  void send_P(int code, PGM_P contentType, PGM_P content) {    
    if (!this->currentClient.has_value()) return;
    const int32_t fd = this->currentClient.value();

    const auto codeStr = std::to_string(code);
    const std::string codeText = iop::httpCodeToString(code);
    if (write(fd, "HTTP/1.0 ", 9) <= 0) iop_assert(false, F(""));
    if (write(fd, codeStr.c_str(), codeStr.length()) <= 0) iop_assert(false, F(""));
    if (write(fd, " ", 1) <= 0) iop_assert(false, F(""));
    if (write(fd, codeText.c_str(), codeText.length()) <= 0) iop_assert(false, F(""));
    if (write(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    if (write(fd, "Content-Type: ", 14) <= 0) iop_assert(false, F(""));
    if (write(fd, contentType, strlen(contentType)) <= 0) iop_assert(false, F(""));
    if (write(fd, "; charset=ISO-8859-5\r\n", 22) <= 0) iop_assert(false, F(""));
    if (this->currentHeaders.length() > 0) {
      if (write(fd, this->currentHeaders.c_str(), this->currentHeaders.length()) <= 0) iop_assert(false, F(""));
    }
    if (this->currentContentLength.has_value()) {
      const auto contentLength = std::to_string(this->currentContentLength.value());
      if (write(fd, "Content-Length: ", 16) <= 0) iop_assert(false, F(""));
      if (write(fd, contentLength.c_str(), contentLength.length()) <= 0) iop_assert(false, F(""));
      if (write(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    }
    if (write(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    if (strlen(content) > 0)
      if (write(fd, content, strlen(content)) <= 0) iop_assert(false, F(""));
  }
  void setContentLength(const size_t contentLength) {
    this->currentContentLength = std::make_optional(contentLength);
  }
  void sendHeader(const iop::StaticString name, const iop::StaticString value, bool first = false) {
    (void) first;
    this->currentHeaders += name.toStdString() + ": " + value.toStdString() + "\r\n";
  }
  void sendContent_P(PGM_P content) {
    if (!this->currentClient.has_value()) return;
    const int32_t fd = this->currentClient.value();
    std::cout << "Content (" << strlen(content) << "): " << content << std::endl;
    if (write(fd, content, strlen(content)) <= 0) return;
    std::cout << "SA" << std::endl;
  }
};

#endif