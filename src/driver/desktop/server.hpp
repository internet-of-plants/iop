#include "driver/server.hpp"
#include "driver/log.hpp"

#include "driver/panic.hpp"

#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>

// TODO: Make it multiplatform
// Berkeley sockets, so assumes POSIX compliant OS //
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <thread>
#include <chrono>

static size_t send(uint32_t fd, const char * msg, const size_t len) noexcept {
  if (iop::Log::isTracing()) iop::Log::print(msg, iop::LogLevel::TRACE, iop::LogType::STARTEND);
  ssize_t sent = 0;
  uint64_t count = 0;
  while ((sent = write(fd, msg, len)) < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    iop_assert(count++ < 1000000, IOP_STATIC_STRING("Waited too long"));
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(50ms);
  }
  iop_assert(sent > 0, std::string("Sent failed (") + std::to_string(sent) + ") [" + std::to_string(errno) + "] " +  strerror(errno) + ": " + msg);
  return sent;
}

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

namespace driver {
iop::Log & logger() noexcept {
  static iop::Log logger_(iop::LogLevel::WARN, IOP_STATIC_STRING("HTTP Server"));
  return logger_;
}

HttpServer::HttpServer(const uint32_t port) noexcept: port(port) {
  this->notFoundHandler = [](HttpConnection &conn, iop::Log const &logger) {
    conn.send(404, IOP_STATIC_STRING("text/plain"), IOP_STATIC_STRING("Not Found"));
    (void) logger;
  };
}
void HttpServer::begin() noexcept {
  IOP_TRACE();
  this->close();

  int32_t fd = 0;
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
    logger().error(IOP_STATIC_STRING("Unable to open socket"));
    return;
  }
  
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    logger().error(IOP_STATIC_STRING("fnctl get failed: "), std::to_string(flags));
    return;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
    logger().error(IOP_STATIC_STRING("fnctl set failed"));
    return;
  }

  this->maybeFD = fd;

  // Posix boilerplate
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(this->port));
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (bind(fd, (struct sockaddr* )&addr, sizeof(addr)) < 0) {
    iop_panic(std::string("Unable to bind socket (") + std::to_string(errno) + "): " + strerror(errno));
    return;
  }

  if (listen(fd, 100) < 0) {
    logger().error(IOP_STATIC_STRING("Unable to listen socket"));
    return;
  }
  logger().info(IOP_STATIC_STRING("Listening to port "), std::to_string(this->port));

  this->address = new (std::nothrow) sockaddr_in(addr);
  iop_assert(this->address, IOP_STATIC_STRING("OOM"));
}
void HttpServer::handleClient() noexcept {
  IOP_TRACE();
  if (!this->address)
    return;

  iop_assert(!this->isHandlingRequest, IOP_STATIC_STRING("Already handling a request"));
  this->isHandlingRequest = true;

  iop_assert(this->maybeFD, IOP_STATIC_STRING("FD not found"));
  int32_t fd = *this->maybeFD;

  sockaddr_in addr = *this->address;
  socklen_t addr_len = sizeof(addr);

  HttpConnection conn;
  int32_t client = 0;
  if ((client = accept(fd, (sockaddr *)&addr, &addr_len)) <= 0) {
    if (client == 0) {
      logger().error(IOP_STATIC_STRING("Client fd is zero"));
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
      logger().error(IOP_STATIC_STRING("Error accepting connection ("), std::to_string(errno), IOP_STATIC_STRING("): "), strerror(errno));
    }
    this->isHandlingRequest = false;
    return;
  }
  logger().debug(IOP_STATIC_STRING("Accepted connection: "), std::to_string(client));
  conn.currentClient = client;

  bool firstLine = true;
  bool isPayload = false;
  
  auto buffer = HttpConnection::Buffer({0});
  auto *start = buffer.data();
  while (true) {
    logger().debug(IOP_STATIC_STRING("Try read: "), std::to_string(strnlen(buffer.begin(), 1024)));

    ssize_t len = 0;
    start += strnlen(buffer.begin(), 1024);
    if (strnlen(buffer.begin(), 1024) < buffer.max_size() &&
        (len = read(client, start, buffer.max_size() - strnlen(buffer.begin(), 1024))) < 0) {
      logger().error(IOP_STATIC_STRING("Read error: "), std::to_string(len));
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
        continue;
      } else {
        logger().error(IOP_STATIC_STRING("Error reading from socket: "), std::to_string(errno), IOP_STATIC_STRING("): "), strerror(errno));
        conn.reset();
        this->isHandlingRequest = false;
        return;
      }
    }
    logger().debug(IOP_STATIC_STRING("Len: "), std::to_string(len));
    if (firstLine == true && len == 0) {
      logger().error(IOP_STATIC_STRING("Empty request"));
      conn.reset();
      this->isHandlingRequest = false;
      return;
    }
    logger().debug(IOP_STATIC_STRING("Read: ("), std::to_string(len), IOP_STATIC_STRING(") ["), std::to_string(strnlen(buffer.begin(), 1024)));

    iop::StringView buff(buffer.begin());
    if (len > 0 && firstLine) {
      if (buff.find("POST") != buff.npos) {
        const ssize_t space = iop::StringView(buff.begin() + 5).find(" ");
        conn.currentRoute = std::string(buff.begin() + 5, space);
        logger().debug(IOP_STATIC_STRING("POST: "), conn.currentRoute);
      } else if (buff.find("GET") != buff.npos) {
        const ssize_t space = iop::StringView(buff.begin() + 4).find(" ");
        conn.currentRoute = std::string(buff.begin() + 4, space);
        logger().debug(IOP_STATIC_STRING("GET: "), conn.currentRoute);
      } else if (buff.find("OPTIONS") != buff.npos) {
        const ssize_t space = iop::StringView(buff.begin() + 7).find(" ");
        conn.currentRoute = std::string(buff.begin() + 7, space);
        logger().debug(IOP_STATIC_STRING("OPTIONS: "), conn.currentRoute);
      } else {
        logger().error(IOP_STATIC_STRING("HTTP Method not found: "), buff);
        conn.reset();
        this->isHandlingRequest = false;
        return;
      }
      firstLine = false;
      
      iop_assert(buff.find("\n") != buff.npos, IOP_STATIC_STRING("First: ").toString() + std::to_string(buff.length()) + IOP_STATIC_STRING(" bytes don't contain newline, the path is too long\n").toString());
      logger().debug(IOP_STATIC_STRING("Found first line"));
      const char* ptr = buff.begin() + buff.find("\n") + 1;
      memmove(buffer.data(), ptr, strlen(ptr) + 1);
      buff = buffer.begin();
    }
    logger().debug(IOP_STATIC_STRING("Headers + Payload: "), buff);

    while (len > 0 && buff.length() > 0 && !isPayload) {
      // TODO: if empty line is split into two reads (because of buff len) we are screwed
      //  || buff.contains(IOP_STATIC_STRING("\n\n")) || buff.contains(IOP_STATIC_STRING("\n\r\n"))
      if (buff.find("\r\n") != buff.npos && buff.find("\r\n") == buff.find("\r\n\r\n")) {
        isPayload = true;

        const char* ptr = buff.begin() + buff.find("\r\n\r\n") + 4;
        memmove(buffer.data(), ptr, strlen(ptr) + 1);
        buff = buffer.data();
        if (buff.find("\n") == buff.npos) continue;
      } else if (buff.find("\r\n") == buff.npos) {
        iop_panic(IOP_STATIC_STRING("Bad code"));
      } else {
        const char* ptr = buff.begin() + buff.find("\r\n") + 2;
        memmove(buffer.data(), ptr, strlen(ptr) + 1);
        buff = buffer.begin();
        // TODO: could this enter in a infinite loop?
        if (buff.find("\n") == buff.npos) continue;
      }
    }

    logger().debug(IOP_STATIC_STRING("Payload ("), std::to_string(buff.length()), IOP_STATIC_STRING(") ["), std::to_string(len), IOP_STATIC_STRING("]: "), buff);

    conn.currentPayload += buff;

    // TODO: For some weird reason `read` is blocking if we try to read after EOF
    // But it's not clear what will happen if EOF is exactly at buffer.max_size()
    // Because it will continue, altough there isn't anything, but it's blocking on EOF
    // But this behavior is not documented. To replicate remove:
    // ` && len == buffer.max_size()` and it will get stuck in the `read` above
    if (len > 0 && buff.length() > 0 && len == buffer.max_size())
      continue;

    logger().debug(IOP_STATIC_STRING("Route: "), conn.currentRoute);
    iop::Log::shouldFlush(false);
    if (this->router.count(conn.currentRoute) != 0) {
      this->router.at(conn.currentRoute)(conn, logger());
    } else {
      logger().debug(IOP_STATIC_STRING("Route not found"));
      this->notFoundHandler(conn, logger());
    }
    iop::Log::shouldFlush(true);
    logger().flush();
    break;
  }

  logger().debug(IOP_STATIC_STRING("Close connection"));
  conn.reset();
  this->isHandlingRequest = false;
}
void HttpServer::close() noexcept {
  IOP_TRACE();
  if (this->address) {
    delete this->address;
    this->address = nullptr;
  }

  if (this->maybeFD)
    ::close(*this->maybeFD);
}

void HttpServer::on(iop::StaticString uri, HttpServer::Callback handler) noexcept {
  this->router.emplace(std::move(uri.toString()), std::move(handler));
}
//called when handler is not assigned
void HttpServer::onNotFound(HttpServer::Callback fn) noexcept {
  this->notFoundHandler = fn;
}

static auto percentDecode(const iop::StringView input) noexcept -> std::optional<std::string> {
  logger().debug(IOP_STATIC_STRING("Decode: "), input);
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
  const char *in = input.begin();
  while((c = *in++) && static_cast<size_t>(in - input.begin()) <= input.length()) {
    if(c == '%') {
      const auto v1 = tbl[(unsigned char)*in++];
      const auto v2 = tbl[(unsigned char)*in++];
      if(v1 < 0 || v2 < 0)
        return std::optional<std::string>();
      c = static_cast<char>((v1 << 4) | v2);
    }
    out.push_back(c);
  }
  return out;
}

void HttpConnection::reset() noexcept {
  this->currentHeaders = "";
  this->currentPayload = "";
  this->currentContentLength.reset();
  if (this->currentClient)
    ::close(*this->currentClient);
}
std::optional<std::string> HttpConnection::arg(const iop::StaticString name) const noexcept {
  IOP_TRACE();
  iop::StringView view(this->currentPayload);

  size_t argEncodingLen = 1 + name.length(); // len(name) + len('=')
  size_t index = view.find(name.toString() + "=");
  if (index != 0) index = view.find(std::string("&") + name.asCharPtr() + "=");
  if (index == view.npos) return std::optional<std::string>();
  if (index != 0) argEncodingLen++; // + len('&')
  view = view.substr(index + argEncodingLen);

  const size_t end = view.find("&");

  if (end == view.npos) {
    const auto decoded = percentDecode(view);
    logger().debug(decoded.value_or("No value"));
    return decoded;
  }

  const auto msg = view.substr(0, end);
  const auto decoded = percentDecode(msg);
  logger().debug(decoded.value_or("No value"));
  return decoded;
}

void HttpConnection::send(uint16_t code, iop::StaticString contentType, iop::StaticString content) const noexcept {
  IOP_TRACE(); 
  iop_assert(this->currentClient, IOP_STATIC_STRING("send but has no client"));
  const int32_t fd = *this->currentClient;

  const auto codeStr = std::to_string(code);
  const std::string codeText = httpCodeToString(code);
  if (iop::Log::isTracing())
    iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::START);
  ::send(fd, "HTTP/1.0 ", 9);
  ::send(fd, codeStr.c_str(), codeStr.length());
  ::send(fd, " ", 1);
  ::send(fd, codeText.c_str(), codeText.length());
  ::send(fd, "\r\n", 2);
  ::send(fd, "Content-Type: ", 14);
  ::send(fd, contentType.asCharPtr(), contentType.length());
  ::send(fd, "; charset=ISO-8859-5\r\n", 22);
  if (this->currentHeaders.length() > 0) {
    ::send(fd, this->currentHeaders.c_str(), this->currentHeaders.length());
  }
  if (this->currentContentLength) {
    const auto contentLength = std::to_string(*this->currentContentLength);
    ::send(fd, "Content-Length: ", 16);
    ::send(fd, contentLength.c_str(), contentLength.length());
    ::send(fd, "\r\n", 2);
  }
  ::send(fd, "\r\n", 2);
  if (content.length() > 0) {
    ::send(fd, content.asCharPtr(), content.length());
    if (iop::Log::isTracing())
      iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::END);
  }
}

void HttpConnection::setContentLength(const size_t contentLength) noexcept {
  IOP_TRACE();
  this->currentContentLength = contentLength;
}
void HttpConnection::sendHeader(const iop::StaticString name, const iop::StaticString value) noexcept{
  IOP_TRACE();
  this->currentHeaders += name.toString() + ": " + value.toString() + "\r\n";
}
void HttpConnection::sendData(iop::StaticString content) const noexcept {
  IOP_TRACE();
  if (!this->currentClient) return;
  const int32_t fd = *this->currentClient;
  logger().debug(IOP_STATIC_STRING("Send Content ("), std::to_string(content.length()), IOP_STATIC_STRING("): "), content);
  
  if (iop::Log::isTracing())
    iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::START);
  ::send(fd, content.asCharPtr(), content.length());
  if (iop::Log::isTracing())
    iop::Log::print(IOP_STATIC_STRING(""), iop::LogLevel::TRACE, iop::LogType::END);
}
void CaptivePortal::start() noexcept {}
void CaptivePortal::close() noexcept {}
void CaptivePortal::handleClient() const noexcept {}
}