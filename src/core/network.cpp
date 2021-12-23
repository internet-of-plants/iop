#include "core/network.hpp"

#ifdef IOP_ONLINE

#include "driver/wifi.hpp"
#include "driver/device.hpp"
#include "driver/thread.hpp"
#include "driver/client.hpp"
#include "core/panic.hpp"
#include "driver/cert_store.hpp"
#include "string.h"
#include "loop.hpp"

constexpr static iop::UpgradeHook defaultHook(iop::UpgradeHook::defaultHook);

static iop::UpgradeHook hook(defaultHook);
static driver::CertStore * maybeCertStore = nullptr;;

namespace iop {
void UpgradeHook::defaultHook() noexcept { IOP_TRACE(); }
void Network::setCertStore(driver::CertStore &store) noexcept {
  maybeCertStore = &store;
}
void Network::setUpgradeHook(UpgradeHook scheduler) noexcept {
  hook = std::move(scheduler);
}
auto Network::takeUpgradeHook() noexcept -> UpgradeHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}

auto Network::isConnected() noexcept -> bool {
  return iop::data.wifi.status() == driver::StationStatus::GOT_IP;
}

void Network::disconnect() noexcept {
  IOP_TRACE();
  return iop::data.wifi.stationDisconnect();
}

static bool initialized = false;
void Network::setup() const noexcept {
  IOP_TRACE();
  if (initialized)
    return;
  initialized = true;

  const char * headers[] = {"LATEST_VERSION",};
  iop::data.http.headersToCollect(headers, 1);

  iop::data.wifi.setup(maybeCertStore);
  iop::Network::disconnect();
  iop::data.wifi.setMode(driver::WiFiMode::STA);

  driver::thisThread.sleep(1);
}

static auto methodToString(const HttpMethod &method) noexcept -> StaticString {
  IOP_TRACE();
  switch (method) {
  case HttpMethod::GET:
    return FLASH("GET");

  case HttpMethod::HEAD:
    return FLASH("HEAD");

  case HttpMethod::POST:
    return FLASH("POST");

  case HttpMethod::PUT:
    return FLASH("PUT");

  case HttpMethod::PATCH:
    return FLASH("PATCH");

  case HttpMethod::DELETE:
    return FLASH("DELETE");

  case HttpMethod::CONNECT:
    return FLASH("CONNECT");
    
  case HttpMethod::OPTIONS:
    return FLASH("OPTIONS");
  }
  iop_panic(FLASH("HTTP Method not found"));
}

// Returns Response if it can understand what the server sent, int is the raw
// response given by ESP8266HTTPClient
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<std::string_view> &token, StaticString path,
                          const std::optional<std::string_view> &data) const noexcept
    -> std::variant<Response, int> {
  IOP_TRACE();
  Network::setup();

  const auto uri = this->uri().toString() + path.toString();
  const auto method = methodToString(method_);

  const auto data_ = data.value_or(std::string_view());

  this->logger.debug(method, FLASH(" to "), this->uri(), path, FLASH(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at info because of that, right
  if (data)
    this->logger.debug(*data);
  
  logMemory(this->logger);

  this->logger.debug(FLASH("Begin"));
  auto maybeSession = iop::data.http.begin(uri);
  if (!maybeSession) {
    this->logger.warn(FLASH("Failed to begin http connection to "), iop::to_view(uri));
    return Response(NetworkStatus::CONNECTION_ISSUES);
  }
  auto &session = *maybeSession;
  this->logger.trace(FLASH("Began HTTP connection"));

  if (token) {
    const auto tok = *token;
    session.setAuthorization(std::string(tok).c_str());
  }

  // Currently only JSON is supported
  if (data)
    session.addHeader(FLASH("Content-Type"), FLASH("application/json"));

  // Authentication headers, identifies device and detects updates, perf
  // monitoring
  {
    auto str = iop::to_view(driver::device.binaryMD5());
    session.addHeader(FLASH("VERSION"), std::string(str));
    str = iop::to_view(driver::device.macAddress());
    session.addHeader(FLASH("MAC_ADDRESS"), std::string(str));
  }
 
  session.addHeader(FLASH("FREE_STACK"), std::to_string(driver::device.availableStack()));
  {
    driver::HeapSelectDram guard;
    session.addHeader(FLASH("FREE_DRAM"), std::to_string(driver::device.availableHeap()));
    session.addHeader(FLASH("BIGGEST_DRAM_BLOCK"), std::to_string(driver::device.biggestHeapBlock()));
  }
  {
    driver::HeapSelectIram guard;
    session.addHeader(FLASH("FREE_IRAM"), std::to_string(driver::device.availableHeap()));
    session.addHeader(FLASH("BIGGEST_IRAM_BLOCK"), std::to_string(driver::device.biggestHeapBlock()));
  }
  session.addHeader(FLASH("VCC"), std::to_string(driver::device.vcc()));
  session.addHeader(FLASH("TIME_RUNNING"), std::to_string(driver::thisThread.now()));
  session.addHeader(FLASH("ORIGIN"), this->uri());
  session.addHeader(FLASH("DRIVER"), driver::device.platform());

  const uint8_t *const data__ = reinterpret_cast<const uint8_t *>(data_.begin());

  this->logger.debug(FLASH("Making HTTP request"));

  auto responseVariant = session.sendRequest(method.toString().c_str(), data__, data_.length());
  if (const auto *error = std::get_if<int>(&responseVariant)) {
    return *error;
  } else if (auto *response = std::get_if<driver::Response>(&responseVariant)) {
    this->logger.debug(FLASH("Made HTTP request")); 

    // Handle system upgrade request
    const auto upgrade = response->header(FLASH("LATEST_VERSION"));
    if (upgrade.length() > 0 && memcmp(upgrade.c_str(), driver::device.binaryMD5().data(), 32) != 0) {
      this->logger.info(FLASH("Scheduled upgrade"));
      hook.schedule();
    }

    // TODO: move this to inside driver::HTTPClient logic
    const auto rawStatus = driver::rawStatus(response->status());
    if (rawStatus == driver::RawStatus::UNKNOWN) {
      this->logger.warn(FLASH("Unknown response code: "), std::to_string(response->status()));
    }
    
    const auto rawStatusStr = driver::rawStatusToString(rawStatus);

    this->logger.debug(FLASH("Response code ("), iop::to_view(std::to_string(response->status())), FLASH("): "), rawStatusStr);

    // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
    /*
    constexpr const int32_t maxPayloadSizeAcceptable = 2048;
    if (unused4KbSysStack.http().getSize() > maxPayloadSizeAcceptable) {
      unused4KbSysStack.http().end();
      const auto lengthStr = std::to_string(response.payload.length());
      this->logger.error(FLASH("Payload from server was too big: "), lengthStr);
      unused4KbSysStack.response() = Response(NetworkStatus::BROKEN_SERVER);
      return unused4KbSysStack.response();
    }
    */

    // We have to simplify the errors reported by this API (but they are logged)
    const auto maybeApiStatus = this->apiStatus(rawStatus);
    if (maybeApiStatus) {
      // The payload is always downloaded, since we check for its size and the
      // origin is trusted. If it's there it's supposed to be there.
      auto payload = response->await().payload;
      this->logger.debug(FLASH("Payload (") , std::to_string(payload.length()), FLASH("): "), iop::to_view(payload));
      // TODO: every response occupies 2x the size because we convert String -> std::string
      return Response(*maybeApiStatus, std::string(payload));
    }
    return response->status();
  }
  iop_panic(FLASH("Invalid variant types"));
}
#else
#include "driver/thread.hpp"
#include "driver/wifi.hpp"

namespace iop {
void Network::setup() const noexcept {
  (void)*this;
  IOP_TRACE();
  WiFi.mode(WIFI_OFF);
  driver::thisThread.sleep(1);
}
void Network::disconnect() noexcept { IOP_TRACE(); }
auto Network::isConnected() noexcept -> bool {
  IOP_TRACE();
  return true;
}
auto Network::httpRequest(const HttpMethod method,
                          const std::optional<std::string_view> &token, std::string_view path,
                          const std::optional<std::string_view> &data) const noexcept
    -> std::variant<Response, int> const &
  (void)*this;
  (void)token;
  (void)method;
  (void)std::move(path);
  (void)data;
  IOP_TRACE();
  return Response(NetworkStatus::OK);
}
#endif

auto Network::httpPost(std::string_view token, const StaticString path, std::string_view data) const noexcept -> std::variant<Response, int> {
  return this->httpRequest(HttpMethod::POST, token, path, data);
}

auto Network::httpPost(StaticString path, std::string_view data) const noexcept -> std::variant<Response, int> {
  return this->httpRequest(HttpMethod::POST, std::nullopt, path, data);
}

auto Network::apiStatusToString(const NetworkStatus &status) noexcept -> StaticString {
  IOP_TRACE();
  switch (status) {
  case NetworkStatus::CONNECTION_ISSUES:
    return FLASH("CONNECTION_ISSUES");
  case NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    return FLASH("CLIENT_BUFFER_OVERFLOW");
  case NetworkStatus::BROKEN_SERVER:
    return FLASH("BROKEN_SERVER");
  case NetworkStatus::OK:
    return FLASH("OK");
  case NetworkStatus::FORBIDDEN:
    return FLASH("FORBIDDEN");
  }
  return FLASH("UNKNOWN");
}

auto Network::apiStatus(const driver::RawStatus &raw) const noexcept -> std::optional<NetworkStatus> {
  IOP_TRACE();
  switch (raw) {
  case driver::RawStatus::CONNECTION_FAILED:
  case driver::RawStatus::CONNECTION_LOST:
    this->logger.warn(FLASH("Connection failed. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::CONNECTION_ISSUES;

  case driver::RawStatus::SEND_FAILED:
  case driver::RawStatus::READ_FAILED:
    this->logger.warn(FLASH("Pipe is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::CONNECTION_ISSUES;

  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
  case driver::RawStatus::NO_SERVER:
  case driver::RawStatus::SERVER_ERROR:
    this->logger.error(FLASH("Server is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::BROKEN_SERVER;

  case driver::RawStatus::READ_TIMEOUT:
    this->logger.warn(FLASH("Network timeout triggered"));
    return NetworkStatus::CONNECTION_ISSUES;

  case driver::RawStatus::OK:
    return NetworkStatus::OK;

  case driver::RawStatus::FORBIDDEN:
    return NetworkStatus::FORBIDDEN;

  case driver::RawStatus::UNKNOWN:
    break;
  }
  return std::nullopt;
}

Network::~Network() noexcept { IOP_TRACE(); }
Network::Network(Network const &other) : logger(other.logger), uri_(other.uri_) { IOP_TRACE(); }
Network::Network(StaticString uri, const LogLevel &logLevel) noexcept
  : logger(logLevel, FLASH("NETWORK")), uri_(std::move(uri)) {
  IOP_TRACE();
}

Response::Response(const NetworkStatus &status) noexcept
    : status(status), payload(std::optional<std::string>()) {
  IOP_TRACE();
}
Response::Response(const NetworkStatus &status, std::string payload) noexcept
    : status(status), payload(payload) {
  IOP_TRACE();
}
Response::Response(Response &&resp) noexcept
    : status(resp.status), payload(std::optional<std::string>()) {
  IOP_TRACE();
  this->payload.swap(resp.payload);
}
auto Response::operator=(Response &&resp) noexcept -> Response & {
  IOP_TRACE();
  this->status = resp.status;
  this->payload.swap(resp.payload);
  return *this;
}

Response::~Response() noexcept {
  IOP_TRACE();
  if (!Log::isTracing())
    return;
  const auto str = Network::apiStatusToString(status);
  Log::print(FLASH("~Response("), LogLevel::TRACE, LogType::START);
  Log::print(str.get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(FLASH(")\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
} // namespace iop