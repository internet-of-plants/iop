#ifndef IOP_DRIVER_SERVER
#define IOP_DRIVER_SERVER

#ifndef IOP_DESKTOP
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <user_interface.h>
#else
#include "core/string/static.hpp"
#include "core/string/cow.hpp"
enum class DNSReplyCode
{
  NoError = 0,
  FormError = 1,
  ServerFailure = 2,
  NonExistentDomain = 3,
  NotImplemented = 4,
  Refused = 5,
  YXDomain = 6,
  YXRRSet = 7,
  NXRRSet = 8
};
class DNSServer {
public:
  void setErrorReplyCode(DNSReplyCode code) {
    (void) code;
  }
  void stop() {}
  void start(uint32_t port, const __FlashStringHelper *match, iop::CowString ip) {
    (void) port;
    (void) match;
    (void) ip;
  }
  void processNextRequest() {}
};

#include "driver/wifi.hpp"

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "core/string/fixed.hpp"
#include <optional>
#include "core/log.hpp"
#include "core/utils.hpp"

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

const static iop::Log logger(iop::LogLevel::DEBUG, F("HTTP Server"));

static void close_(uint32_t fd) noexcept {
  close(fd);
}

static int send_(uint32_t fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing()) iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  int sent = 0;
  uint64_t count = 0;
  while ((sent = write(fd, msg, len)) < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    iop_assert(count++ < 1000000, F("Waited too long"));
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
  }
  iop_assert(sent > 0, std::string("Sent failed (") + std::to_string(sent) + ") [" + std::to_string(errno) + "] " +  strerror(errno) + ": " + msg);
  return sent;
}

namespace iop {
static std::string httpCodeToString(const int code) {
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
  uint16_t port;

  std::optional<int32_t> maybeFd;
  std::optional<struct sockaddr_in> maybeAddress;

  std::optional<int32_t> currentClient;
  std::string currentHeaders;
  std::string currentPayload;
  std::optional<size_t> currentContentLength;
  std::string currentRoute;

public:
  ESP8266WebServer(uint16_t port = 8082): port(port) {
    this->notFoundHandler = [this]() {
      this->send_P(404, "text/plain", "Not Found");
    };
  }
  ~ESP8266WebServer() {}

  enum class ClientFuture { REQUEST_CAN_CONTINUE, REQUEST_IS_HANDLED, MUST_STOP, IS_GIVEN };

  void begin() {
    IOP_TRACE();
    this->close();

    int32_t fd = 0;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
      logger.error(F("Unable to open socket"));
      return;
    }
    
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
      logger.error(F("fnctl get failed: "), std::to_string(flags));
      return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
      logger.error(F("fnctl set failed"));
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
      logger.error(F("Unable to bind socket ("), std::to_string(errno), F("): "), iop::UnsafeRawString(strerror(errno)));
      return;
    }

    if (listen(fd, 100) < 0) {
      logger.error(F("Unable to listen socket"));
      return;
    }
    logger.info(F("Listening to port "), std::to_string(this->port));

    this->maybeAddress = std::make_optional(address);
  }
  
  void handleClient() {
    IOP_TRACE();
    if (!this->maybeAddress.has_value())
      return;
    this->endRequest();

    int32_t fd = iop::unwrap_ref(this->maybeFd, IOP_CTX());

    struct sockaddr_in address = iop::unwrap_ref(this->maybeAddress, IOP_CTX());
    socklen_t addr_len = sizeof(address);

    int32_t client = 0;
    if ((client = accept(fd, (struct sockaddr *)&address, &addr_len)) <= 0) {
      if (client == 0) {
        logger.error(F("Client fd is zero"));
      } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        logger.error(F("Error accepting connection ("), std::to_string(errno), F("): "), iop::UnsafeRawString(strerror(errno)));
      }
      return;
    }
    logger.debug(F("Accepted connection: "), std::to_string(client));
    this->currentClient = std::make_optional(client);

    bool firstLine = true;
    bool isPayload = false;
    
    auto buffer = Buffer::empty();
    auto *start = buffer.asMut();
    while (true) {
      logger.debug(F("Try read: "), std::to_string(buffer.length()));

      ssize_t len = 0;
      start += buffer.length();
      if (buffer.length() < buffer.size &&
          (len = read(client, start, buffer.size - buffer.length())) < 0) {
        logger.error(F("Read error: "), std::to_string(len));
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          using namespace std::chrono_literals;
          std::this_thread::sleep_for(50ms);
          continue;
        } else {
          logger.error(F("Error reading from socket: "), std::to_string(errno), F("): "), iop::UnsafeRawString(strerror(errno)));
          this->endRequest();
          return;
        }
      }
      logger.debug(F("Len: "), std::to_string(len));
      if (firstLine == true && len == 0) {
        logger.error(F("Empty request"));
        this->endRequest();
        return;
      }
      logger.debug(F("Read: ("), std::to_string(len), F(") ["), std::to_string(buffer.length()));

      iop::StringView buff(buffer);
      if (len > 0 && firstLine) {
        if (buff.contains(F("POST"))) {
          const ssize_t space = iop::StringView(iop::UnsafeRawString(buff.get() + 5)).indexOf(F(" "));
          this->currentRoute = std::string(buff.get() + 5, space);
          logger.debug(F("POST: "), this->currentRoute);
        } else if (buff.contains(F("GET"))) {
          const ssize_t space = iop::StringView(iop::UnsafeRawString(buff.get() + 4)).indexOf(F(" "));
          this->currentRoute = std::string(buff.get() + 4, space);
          logger.debug(F("GET: "), this->currentRoute);
        } else if (buff.contains(F("OPTIONS"))) {
          const ssize_t space = iop::StringView(iop::UnsafeRawString(buff.get() + 7)).indexOf(F(" "));
          this->currentRoute = std::string(buff.get() + 7, space);
          logger.debug(F("OPTIONS: "), this->currentRoute);
        } else {
          logger.error(F("HTTP Method not found: "), buff);
          this->endRequest();
          return;
        }
        firstLine = false;
        
        iop_assert(buff.contains(F("\n")), iop::StaticString(F("First: ")).toStdString() + std::to_string(buff.length()) + iop::StaticString(F(" bytes don't contain newline, the path is too long\n")).toStdString());
        logger.debug(F("Found first line"));
        const char* ptr = buff.get() + buff.indexOf(F("\n")) + 1;
        memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
      }
      logger.debug(F("Headers + Payload: "), buff);

      while (len > 0 && buff.length() > 0 && !isPayload) {
        // TODO: if empty line is split into two reads (because of buff len) we are screwed
        //  || buff.contains(F("\n\n")) || buff.contains(F("\n\r\n"))
        if (buff.contains(F("\r\n")) && buff.indexOf(F("\r\n")) == buff.indexOf(F("\r\n\r\n"))) {
          isPayload = true;

          const char* ptr = buff.get() + buff.indexOf(F("\r\n\r\n")) + 4;
          memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
          if (!buff.contains(F("\n"))) continue;
        } else if (!buff.contains(F("\r\n"))) {
          iop_panic(F("Bad code"));
        } else {
          const char* ptr = buff.get() + buff.indexOf(F("\r\n")) + 2;
          memmove(buffer.asMut(), ptr, strlen(ptr) + 1);
          // TODO: could this enter in a infinite loop?
          if (!buff.contains(F("\n"))) continue;
        }
      }

      logger.debug(F("Payload ("), std::to_string(buff.length()), F(") ["), std::to_string(len), F("]: "), buffer);

      this->currentPayload += buff.get();

      // TODO: For some weird reason `read` is blocking if we try to read after EOF
      // But it's not clear what will happen if EOF is exactly at buffer.size
      // Because it will continue, altough there isn't anything, but it's blocking on EOF
      // But this behavior is not documented. To replicate remove:
      // ` && len == buffer.size` and it will get stuck in the `read` above
      if (len > 0 && buff.length() > 0 && len == buffer.size)
        continue;

      logger.debug(F("Route: "), this->currentRoute);
      if (this->router.count(this->currentRoute) != 0) {
        this->router.at(this->currentRoute)();
      } else {
        logger.debug(F("Route not found"));
        this->notFoundHandler();
      }
      break;
    }

    logger.debug(F("Close connection"));
    this->endRequest();
  }

  void endRequest() {
    this->currentHeaders = "";
    this->currentPayload = "";
    this->currentContentLength.reset();
    if (this->currentClient.has_value()) {
      close_(iop::unwrap(this->currentClient, IOP_CTX()));
    }
  }

  void close() {
    IOP_TRACE();
    this->maybeAddress.reset();
    this->endRequest();

    if (this->maybeFd.has_value()) {
      close_(iop::unwrap(this->maybeFd, IOP_CTX()));
    }
  }

  void on(iop::StaticString uri, std::function<void(void)> handler) {
    this->router.emplace(std::move(uri.toStdString()), std::move(handler));
  }
  //called when handler is not assigned
  void onNotFound(std::function<void(void)> fn) {
    this->notFoundHandler = fn;
  }


  static auto percentDecode(const iop::StringView input) -> std::optional<std::string> {
    logger.debug(F("Decode: "), input);
    static const char tbl[256] = {
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
      -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
    };
    std::string out;
    out.reserve(input.length());
    char c = '\0';
    const char *in = input.get();
    while((c = *in++) != '\0') {
      if(c == '%') {
        const auto v1 = tbl[(unsigned char)*in++];
        const auto v2 = tbl[(unsigned char)*in++];
        if(v1 < 0 || v2 < 0) {
          return std::optional<std::string>();
        }
        c = (v1 << 4) | v2;
      }
      out.push_back(c);
    }
    return std::make_optional(out);
  }

  // get request argument value by name
  std::string arg(iop::StaticString name) const {
    IOP_TRACE();
    iop::StringView view(this->currentPayload);

    const size_t argEncodingLen = 2 + name.length(); // len('&') + len('=') + len(name)
    ssize_t index = view.indexOf(std::string("&") + name.asCharPtr() + "=");
    if (index < 0) index = view.indexOf(std::string("?") + name.asCharPtr() + "=");
    if (index < 0) return "";

    const ssize_t end = iop::StringView(iop::UnsafeRawString(view.get() + index + argEncodingLen)).indexOf(F("&"));

    if (end < 0) {
      const auto msg = iop::UnsafeRawString(view.get() + index + argEncodingLen);
      const auto decoded = ESP8266WebServer::percentDecode(msg);
      logger.debug(decoded.has_value() ? decoded.value() : "No value");
      return decoded.value_or("");
    }

    const auto msg = std::string(view.get() + index + argEncodingLen, end);
    const auto decoded = ESP8266WebServer::percentDecode(msg);
    logger.debug(decoded.has_value() ? decoded.value() : "No value");
    return decoded.value_or("");
  }
  // check if argument exists
  bool hasArg(iop::StaticString name) const {
    IOP_TRACE();
    iop::StringView view(this->currentPayload);
    const auto ret = view.contains(std::string("&") + name.asCharPtr() + "=")
      || view.contains(std::string("?") + name.asCharPtr() + "=");
    logger.debug(F("Has Arg ("), name, F("): "), std::to_string(ret));
    return ret;
  }

  // send response to the client
  // code - HTTP response code, can be 200 or 404
  // content_type - HTTP content type, like "text/plain" or "image/png"
  // content - actual content body
  
  void send_P(int code, PGM_P contentType, PGM_P content) {   
    IOP_TRACE(); 
    iop_assert(this->currentClient.has_value(), F("send_P but has no client"));
    const int32_t fd = iop::unwrap_ref(this->currentClient, IOP_CTX());

    const auto codeStr = std::to_string(code);
    const std::string codeText = iop::httpCodeToString(code);
    if (iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::START);
    if (send_(fd, "HTTP/1.0 ", 9) <= 0) iop_assert(false, F(""));
    if (send_(fd, codeStr.c_str(), codeStr.length()) <= 0) iop_assert(false, F(""));
    if (send_(fd, " ", 1) <= 0) iop_assert(false, F(""));
    if (send_(fd, codeText.c_str(), codeText.length()) <= 0) iop_assert(false, F(""));
    if (send_(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    if (send_(fd, "Content-Type: ", 14) <= 0) iop_assert(false, F(""));
    if (send_(fd, contentType, strlen(contentType)) <= 0) iop_assert(false, F(""));
    if (send_(fd, "; charset=ISO-8859-5\r\n", 22) <= 0) iop_assert(false, F(""));
    if (this->currentHeaders.length() > 0) {
      if (send_(fd, this->currentHeaders.c_str(), this->currentHeaders.length()) <= 0) iop_assert(false, F(""));
    }
    if (this->currentContentLength.has_value()) {
      const auto contentLength = std::to_string(iop::unwrap_ref(this->currentContentLength, IOP_CTX()));
      if (send_(fd, "Content-Length: ", 16) <= 0) iop_assert(false, F(""));
      if (send_(fd, contentLength.c_str(), contentLength.length()) <= 0) iop_assert(false, F(""));
      if (send_(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    }
    if (send_(fd, "\r\n", 2) <= 0) iop_assert(false, F(""));
    if (strlen(content) > 0)
      if (send_(fd, content, strlen(content)) <= 0) iop_assert(false, F(""));
    if (iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::END);

  }
  void setContentLength(const size_t contentLength) {
    IOP_TRACE();
    this->currentContentLength = std::make_optional(contentLength);
  }
  void sendHeader(const iop::StaticString name, const iop::StaticString value, bool first = false) {
    IOP_TRACE();
    (void) first;
    this->currentHeaders += name.toStdString() + ": " + value.toStdString() + "\r\n";
  }
  void sendContent_P(PGM_P content) {
    IOP_TRACE();
    if (!this->currentClient.has_value()) return;
    const int32_t fd = iop::unwrap_ref(this->currentClient, IOP_CTX());
    logger.debug(F("Send Content ("), std::to_string(strlen(content)), F("): "), iop::UnsafeRawString(content));
    
    if (iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::START);
    send_(fd, content, strlen(content));

    if (iop::Log::isTracing())
      iop::Log::print(F(""), iop::LogLevel::TRACE, iop::LogType::END);
  }
};
#endif

#endif