#include "core/network.hpp"
#include "core/utils.hpp"

#ifdef IOP_ONLINE

#include "driver/wifi.hpp"
#include "driver/device.hpp"
#include "driver/thread.hpp"
#include "driver/client.hpp"
#include "core/panic.hpp"
#include "core/cert_store.hpp"
#include "string.h"
#include "loop.hpp"

constexpr static iop::UpgradeHook defaultHook(iop::UpgradeHook::defaultHook);

static iop::UpgradeHook hook(defaultHook);
static iop::CertStore * maybeCertStore = nullptr;;

namespace iop {
void UpgradeHook::defaultHook() noexcept { IOP_TRACE(); }
void Network::setCertStore(iop::CertStore &store) noexcept {
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
  IOP_TRACE();
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

static auto methodToString(const HttpMethod &method) noexcept
    -> std::optional<StaticString> {
  IOP_TRACE();
  std::optional<StaticString> ret;
  switch (method) {
  case HttpMethod::GET:
    ret.emplace(F("GET"));
    break;
  case HttpMethod::HEAD:
    ret.emplace(F("HEAD"));
    break;
  case HttpMethod::POST:
    ret.emplace(F("POST"));
    break;
  case HttpMethod::PUT:
    ret.emplace(F("PUT"));
    break;
  case HttpMethod::PATCH:
    ret.emplace(F("PATCH"));
    break;
  case HttpMethod::DELETE:
    ret.emplace(F("DELETE"));
    break;
  case HttpMethod::CONNECT:
    ret.emplace(F("CONNECT"));
    break;
  case HttpMethod::OPTIONS:
    ret.emplace(F("OPTIONS"));
    break;
  }
  return ret;
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
  const auto method = iop::unwrap_ref(methodToString(method_), IOP_CTX());

  const auto data_ = data.value_or(std::string_view());

  this->logger.info(method, F(" to "), this->uri(), path, F(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at info because of that, right
  if (data.has_value())
    this->logger.debug(iop::unwrap_ref(data, IOP_CTX()));
  
  logMemory(this->logger);

  this->logger.debug(F("Begin"));
  auto maybeSession = iop::data.http.begin(uri);
  if (!maybeSession.has_value()) {
    this->logger.warn(F("Failed to begin http connection to "), iop::to_view(uri));
    return Response(NetworkStatus::CONNECTION_ISSUES);
  }
  this->logger.trace(F("Began HTTP connection"));
  auto &session = iop::unwrap_mut(maybeSession, IOP_CTX());

  if (token.has_value()) {
    const auto tok = iop::unwrap_ref(token, IOP_CTX());
    session.setAuthorization(std::string(tok).c_str());
  }

  // Currently only JSON is supported
  if (data.has_value())
    session.addHeader(F("Content-Type"), F("application/json"));

  // Authentication headers, identifies device and detects updates, perf
  // monitoring
  {
    auto str = iop::to_view(driver::device.binaryMD5());
    session.addHeader(F("VERSION"), std::string(str));
    str = iop::to_view(driver::device.macAddress());
    session.addHeader(F("MAC_ADDRESS"), std::string(str));
  }
 
  session.addHeader(F("FREE_STACK"), std::to_string(driver::device.availableStack()));
  session.addHeader(F("FREE_HEAP"), std::to_string(driver::device.availableHeap()));
  session.addHeader(F("BIGGEST_FREE_HEAP_BLOCK"), std::to_string(driver::device.biggestHeapBlock()));
  session.addHeader(F("VCC"), std::to_string(driver::device.vcc()));
  session.addHeader(F("TIME_RUNNING"), std::to_string(driver::thisThread.now()));
  session.addHeader(F("ORIGIN"), this->uri());
  session.addHeader(F("DRIVER"), driver::device.platform());

  const uint8_t *const data__ = reinterpret_cast<const uint8_t *>(data_.begin());

  this->logger.debug(F("Making HTTP request"));

  auto responseVariant = session.sendRequest(method.toString().c_str(), data__, data_.length());
  if (iop::is_err(responseVariant)) {
    return iop::unwrap_err_ref(responseVariant, IOP_CTX());
  }
  auto response = iop::unwrap_ok_mut(responseVariant, IOP_CTX());

  this->logger.debug(F("Made HTTP request")); 

  // Handle system upgrade request
  const auto upgrade = response.header(F("LATEST_VERSION"));
  if (upgrade.length() > 0 && memcmp(upgrade.c_str(), driver::device.binaryMD5().data(), 32) != 0) {
    this->logger.info(F("Scheduled upgrade"));
    hook.schedule();
  }

  // TODO: move this to inside driver::HTTPClient logic
  const auto rawStatus = driver::rawStatus(response.status());
  if (rawStatus == driver::RawStatus::UNKNOWN) {
    this->logger.warn(F("Unknown response code: "), std::to_string(response.status()));
  }
  
  const auto rawStatusStr = driver::rawStatusToString(rawStatus);

  this->logger.info(F("Response code ("), std::to_string(response.status()), F("): "), rawStatusStr);

  // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
  /*
  constexpr const int32_t maxPayloadSizeAcceptable = 2048;
  if (unused4KbSysStack.http().getSize() > maxPayloadSizeAcceptable) {
    unused4KbSysStack.http().end();
    const auto lengthStr = std::to_string(response.payload.length());
    this->logger.error(F("Payload from server was too big: "), lengthStr);
    unused4KbSysStack.response() = Response(NetworkStatus::BROKEN_SERVER);
    return unused4KbSysStack.response();
  }
  */

  // We have to simplify the errors reported by this API (but they are logged)
  const auto maybeApiStatus = this->apiStatus(rawStatus);
  if (maybeApiStatus.has_value()) {
    // The payload is always downloaded, since we check for its size and the
    // origin is trusted. If it's there it's supposed to be there.
    auto payload = response.await().payload;
    this->logger.debug(F("Payload (") , std::to_string(payload.length()), F("): "), iop::to_view(payload));
    // TODO: every response occupies 2x the size because we convert String -> std::string
    return Response(iop::unwrap_ref(maybeApiStatus, IOP_CTX()), std::string(payload));
  }
  return response.status();
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

auto Network::httpPost(std::string_view token, const StaticString path,
                       std::string_view data) const noexcept
    -> std::variant<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, std::make_optional(std::move(token)),
                           path,
                           std::make_optional(std::move(data)));
}

auto Network::httpPost(StaticString path, std::string_view data) const noexcept
    -> std::variant<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, std::optional<std::string_view>(),
                           path,
                           std::make_optional(std::move(data)));
}

auto Network::apiStatusToString(const NetworkStatus &status) noexcept
    -> StaticString {
  IOP_TRACE();
  switch (status) {
  case NetworkStatus::CONNECTION_ISSUES:
    return F("CONNECTION_ISSUES");
  case NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    return F("CLIENT_BUFFER_OVERFLOW");
  case NetworkStatus::BROKEN_SERVER:
    return F("BROKEN_SERVER");
  case NetworkStatus::OK:
    return F("OK");
  case NetworkStatus::FORBIDDEN:
    return F("FORBIDDEN");
  }
  return F("UNKNOWN");
}

auto Network::apiStatus(const driver::RawStatus &raw) const noexcept
    -> std::optional<NetworkStatus> {
  std::optional<NetworkStatus> ret;
  IOP_TRACE();
  switch (raw) {
  case driver::RawStatus::CONNECTION_FAILED:
  case driver::RawStatus::CONNECTION_LOST:
    this->logger.warn(F("Connection failed. Code: "),
                      std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case driver::RawStatus::SEND_FAILED:
  case driver::RawStatus::READ_FAILED:
    this->logger.warn(F("Pipe is broken. Code: "),
                      std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
  case driver::RawStatus::NO_SERVER:
  case driver::RawStatus::SERVER_ERROR:
    this->logger.error(F("Server is broken. Code: "),
                       std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::BROKEN_SERVER);
    break;

  case driver::RawStatus::READ_TIMEOUT:
    this->logger.warn(F("Network timeout triggered"));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case driver::RawStatus::OK:
    ret.emplace(NetworkStatus::OK);
    break;
  case driver::RawStatus::FORBIDDEN:
    ret.emplace(NetworkStatus::FORBIDDEN);
    break;

  case driver::RawStatus::UNKNOWN:
    break;
  }
  return ret;
}
auto Network::operator=(Network const &other) -> Network & {
  IOP_TRACE();
  if (this == &other)
    return *this;
  this->uri_ = other.uri_;
  this->logger = other.logger;
  return *this;
}
Network::~Network() noexcept { IOP_TRACE(); }
Network::Network(Network const &other) : logger(other.logger), uri_(other.uri_) { IOP_TRACE(); }
Network::Network(StaticString uri, const LogLevel &logLevel) noexcept
  : logger(logLevel, F("NETWORK")), uri_(std::move(uri)) {
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
  Log::print(F("~Response("), LogLevel::TRACE, LogType::START);
  Log::print(str.get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(")\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
} // namespace iop