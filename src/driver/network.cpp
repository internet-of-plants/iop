#include "driver/network.hpp"
#include "driver/cert_store.hpp"
#include "driver/thread.hpp"

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
    return IOP_STATIC_STRING("CONNECTION_ISSUES");
  case NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    return IOP_STATIC_STRING("CLIENT_BUFFER_OVERFLOW");
  case NetworkStatus::BROKEN_SERVER:
    return IOP_STATIC_STRING("BROKEN_SERVER");
  case NetworkStatus::OK:
    return IOP_STATIC_STRING("OK");
  case NetworkStatus::FORBIDDEN:
    return IOP_STATIC_STRING("FORBIDDEN");
  }
  return IOP_STATIC_STRING("UNKNOWN");
}

auto Network::apiStatus(const driver::RawStatus &raw) const noexcept -> std::optional<NetworkStatus> {
  IOP_TRACE();
  switch (raw) {
  case driver::RawStatus::CONNECTION_FAILED:
  case driver::RawStatus::CONNECTION_LOST:
    this->logger.warn(IOP_STATIC_STRING("Connection failed. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::CONNECTION_ISSUES;

  case driver::RawStatus::SEND_FAILED:
  case driver::RawStatus::READ_FAILED:
    this->logger.warn(IOP_STATIC_STRING("Pipe is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::CONNECTION_ISSUES;

  case driver::RawStatus::ENCODING_NOT_SUPPORTED:
  case driver::RawStatus::NO_SERVER:
  case driver::RawStatus::SERVER_ERROR:
    this->logger.error(IOP_STATIC_STRING("Server is broken. Code: "), std::to_string(static_cast<int>(raw)));
    return NetworkStatus::BROKEN_SERVER;

  case driver::RawStatus::READ_TIMEOUT:
    this->logger.warn(IOP_STATIC_STRING("Network timeout triggered"));
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

Network::Network(StaticString uri, const LogLevel &logLevel) noexcept
  : logger(logLevel, IOP_STATIC_STRING("NETWORK")), uri_(std::move(uri)) {
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
}

#if !defined(IOP_ONLINE) || defined(IOP_NOOP)
#include "driver/noop/network.hpp"
#else

#include "driver/wifi.hpp"
#include "driver/device.hpp"
#include "driver/client.hpp"
#include "driver/panic.hpp"
#include "string.h"

namespace iop {
static auto methodToString(const HttpMethod &method) noexcept -> StaticString;

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

  this->logger.debug(method, IOP_STATIC_STRING(" to "), this->uri(), path, IOP_STATIC_STRING(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at info because of that, right
  if (data)
    this->logger.debug(*data);
  
  logMemory(this->logger);

  this->logger.debug(IOP_STATIC_STRING("Begin"));
  auto maybeSession = iop::data.http.begin(uri);
  if (!maybeSession) {
    this->logger.warn(IOP_STATIC_STRING("Failed to begin http connection to "), iop::to_view(uri));
    return Response(NetworkStatus::CONNECTION_ISSUES);
  }
  auto &session = *maybeSession;
  this->logger.trace(IOP_STATIC_STRING("Began HTTP connection"));

  if (token) {
    const auto tok = *token;
    session.setAuthorization(std::string(tok).c_str());
  }

  // Currently only JSON is supported
  if (data)
    session.addHeader(IOP_STATIC_STRING("Content-Type"), IOP_STATIC_STRING("application/json"));

  // Authentication headers, identifies device and detects updates, perf
  // monitoring
  {
    auto str = iop::to_view(driver::device.binaryMD5());
    session.addHeader(IOP_STATIC_STRING("VERSION"), std::string(str));
    str = iop::to_view(driver::device.macAddress());
    session.addHeader(IOP_STATIC_STRING("MAC_ADDRESS"), std::string(str));
  }
 
  session.addHeader(IOP_STATIC_STRING("FREE_STACK"), std::to_string(driver::device.availableStack()));
  {
    driver::HeapSelectDram guard;
    session.addHeader(IOP_STATIC_STRING("FREE_DRAM"), std::to_string(driver::device.availableHeap()));
    session.addHeader(IOP_STATIC_STRING("BIGGEST_DRAM_BLOCK"), std::to_string(driver::device.biggestHeapBlock()));
  }
  {
    driver::HeapSelectIram guard;
    session.addHeader(IOP_STATIC_STRING("FREE_IRAM"), std::to_string(driver::device.availableHeap()));
    session.addHeader(IOP_STATIC_STRING("BIGGEST_IRAM_BLOCK"), std::to_string(driver::device.biggestHeapBlock()));
  }
  session.addHeader(IOP_STATIC_STRING("VCC"), std::to_string(driver::device.vcc()));
  session.addHeader(IOP_STATIC_STRING("TIME_RUNNING"), std::to_string(driver::thisThread.now()));
  session.addHeader(IOP_STATIC_STRING("ORIGIN"), this->uri());
  session.addHeader(IOP_STATIC_STRING("DRIVER"), driver::device.platform());

  const uint8_t *const data__ = reinterpret_cast<const uint8_t *>(data_.begin());

  this->logger.debug(IOP_STATIC_STRING("Making HTTP request"));

  auto responseVariant = session.sendRequest(method.toString().c_str(), data__, data_.length());
  if (const auto *error = std::get_if<int>(&responseVariant)) {
    return *error;
  } else if (auto *response = std::get_if<driver::Response>(&responseVariant)) {
    this->logger.debug(IOP_STATIC_STRING("Made HTTP request")); 

    // Handle system upgrade request
    const auto upgrade = response->header(IOP_STATIC_STRING("LATEST_VERSION"));
    if (upgrade.length() > 0 && memcmp(upgrade.c_str(), driver::device.binaryMD5().data(), 32) != 0) {
      this->logger.info(IOP_STATIC_STRING("Scheduled upgrade"));
      hook.schedule();
    }

    // TODO: move this to inside driver::HTTPClient logic
    const auto rawStatus = driver::rawStatus(response->status());
    if (rawStatus == driver::RawStatus::UNKNOWN) {
      this->logger.warn(IOP_STATIC_STRING("Unknown response code: "), std::to_string(response->status()));
    }
    
    const auto rawStatusStr = driver::rawStatusToString(rawStatus);

    this->logger.debug(IOP_STATIC_STRING("Response code ("), iop::to_view(std::to_string(response->status())), IOP_STATIC_STRING("): "), rawStatusStr);

    // TODO: this is broken because it's not lazy, it should be a HTTPClient setting that bails out if it's bigger, and encapsulated in the API
    /*
    constexpr const int32_t maxPayloadSizeAcceptable = 2048;
    if (unused4KbSysStack.http().getSize() > maxPayloadSizeAcceptable) {
      unused4KbSysStack.http().end();
      const auto lengthStr = std::to_string(response.payload.length());
      this->logger.error(IOP_STATIC_STRING("Payload from server was too big: "), lengthStr);
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
      this->logger.debug(IOP_STATIC_STRING("Payload (") , std::to_string(payload.length()), IOP_STATIC_STRING("): "), iop::to_view(payload));
      // TODO: every response occupies 2x the size because we convert String -> std::string
      return Response(*maybeApiStatus, std::string(payload));
    }
    return response->status();
  }
  iop_panic(IOP_STATIC_STRING("Invalid variant types"));
}

void Network::setup() const noexcept {
  IOP_TRACE();
  static bool initialized = false;
  if (initialized) return;
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
    return IOP_STATIC_STRING("GET");

  case HttpMethod::HEAD:
    return IOP_STATIC_STRING("HEAD");

  case HttpMethod::POST:
    return IOP_STATIC_STRING("POST");

  case HttpMethod::PUT:
    return IOP_STATIC_STRING("PUT");

  case HttpMethod::PATCH:
    return IOP_STATIC_STRING("PATCH");

  case HttpMethod::DELETE:
    return IOP_STATIC_STRING("DELETE");

  case HttpMethod::CONNECT:
    return IOP_STATIC_STRING("CONNECT");
    
  case HttpMethod::OPTIONS:
    return IOP_STATIC_STRING("OPTIONS");
  }
  iop_panic(IOP_STATIC_STRING("HTTP Method not found"));
}
} // namespace iop
#endif