#include "core/network.hpp"
#include "core/utils.hpp"

#ifdef IOP_ONLINE

#include "driver/device.hpp"
#include "driver/client.hpp"

const static iop::UpgradeHook defaultHook(iop::UpgradeHook::defaultHook);

static iop::UpgradeHook hook(defaultHook);
static std::optional<iop::CertStore> maybeCertStore;

namespace iop {
void UpgradeHook::defaultHook() noexcept { IOP_TRACE(); }

void Network::setCertStore(iop::CertStore store) noexcept {
  maybeCertStore = std::make_optional(std::move(store));
}
void Network::setUpgradeHook(UpgradeHook scheduler) noexcept {
  hook = std::move(scheduler);
}
auto Network::takeUpgradeHook() noexcept -> UpgradeHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}

#ifdef IOP_SSL
static BearSSL::WiFiClientSecure client;
#else
static WiFiClient client;
#endif
static HTTPClient http;

auto Network::isConnected() noexcept -> bool {
  IOP_TRACE();
  return WiFi.status() == WL_CONNECTED;
}

void Network::disconnect() noexcept {
  IOP_TRACE();
  WiFi.disconnect();
}

static bool initialized = false;
auto Network::setup() const noexcept -> void {
  IOP_TRACE();
  if (initialized)
    return;
  initialized = true;

  if (!this->uri().contains(F(":"))) {
    const StaticString error(F("Host must contain protocol (http:// or https://): "));
    iop_panic(error.toStdString() + " " + this->uri().toStdString());
  }

  http.setReuse(false);

  static const char *headers[] = {PSTR("LATEST_VERSION")};
  http.collectHeaders(headers, 1);

  client.setNoDelay(false);
  client.setSync(true);

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
  delay(1);
  WiFi.reconnect();
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

auto Network::wifiClient() noexcept -> WiFiClient & { return client; }

// Returns Response if it can understand what the server sent, int is the raw
// response given by ESP8266HTTPClient
auto Network::httpRequest(const HttpMethod method_,
                          const std::optional<StringView> &token, StringView path,
                          const std::optional<StringView> &data) const noexcept
    -> Result<Response, int> {
  IOP_TRACE();
  Network::setup();

  if (!Network::isConnected())
    return Response(NetworkStatus::CONNECTION_ISSUES);

  #ifdef IOP_DESKTOP
  const auto uri = this->uri().toStdString() + path.get();
  #else
  const auto uri = String(this->uri().get()) + path.get();
  #endif
  const auto method = iop::unwrap_ref(methodToString(method_), IOP_CTX());

  auto data_ = emptyStringView;
  if (data.has_value())
    data_ = iop::unwrap_ref(data, IOP_CTX());

  this->logger.info(method, F(", data length: "), std::to_string(data_.length()));

  // TODO: this may log sensitive information, network logging is currently
  // capped at info because of that, right
  if (data.has_value())
    this->logger.debug(iop::unwrap_ref(data, IOP_CTX()));

  if (token.has_value()) {
    const auto tok = iop::unwrap_ref(token, IOP_CTX());
    this->logger.debug(F("Token: "), tok);
    http.setAuthorization(tok.get());
  } else {
    // We have to clear the authorization, it persists between requests
    http.setAuthorization("");
  }

  // We can afford bigger timeouts since we shouldn't make frequent requests
  constexpr uint32_t oneMinuteMs = 60 * 1000;
  http.setTimeout(oneMinuteMs);

  // Currently only JSON is supported
  if (data.has_value())
    http.addHeader(F("Content-Type"), F("application/json"));

  // Authentication headers, identifies device and detects updates, perf
  // monitoring
  http.addHeader(F("MAC_ADDRESS"), macAddress().asString().get());
  http.addHeader(F("VERSION"), hashSketch().asString().get());
 
  http.addHeader(F("FREE_STACK"), std::to_string(ESP.getFreeContStack()).c_str());
  http.addHeader(F("FREE_HEAP"), std::to_string(ESP.getFreeHeap()).c_str());
  http.addHeader(F("HEAP_FRAGMENTATION"), std::to_string(ESP.getHeapFragmentation()).c_str());
  http.addHeader(F("BIGGEST_FREE_BLOCK"), std::to_string(ESP.getMaxFreeBlockSize()).c_str());

  if (!http.begin(Network::wifiClient(), uri)) {
    this->logger.warn(F("Failed to begin http connection to "), uri);
    return Response(NetworkStatus::CONNECTION_ISSUES);
  }
  this->logger.trace(F("Began HTTP connection"));

  const auto *const data__ = reinterpret_cast<const uint8_t *>(data_.get());

  this->logger.debug(F("Making HTTP request"));
  const auto code =
      http.sendRequest(method.toStdString().c_str(), data__, data_.length());
  this->logger.debug(F("Made HTTP request")); 

  // Handle system upgrade request
  const auto upgrade = http.header(PSTR("LATEST_VERSION"));
  if (upgrade.length() > 0 &&
      strcmp(upgrade.c_str(), hashSketch().asString().get()) != 0) {
    this->logger.info(F("Scheduled upgrade"));
    hook.schedule();
  }

  const auto rawStatus = this->rawStatus(code);
  const auto rawStatusStr = Network::rawStatusToString(rawStatus);

  this->logger.info(F("Response code ("), std::to_string(code), F("): "), rawStatusStr);

  constexpr const int32_t maxPayloadSizeAcceptable = 2048;
  if (http.getSize() > maxPayloadSizeAcceptable) {
    http.end();
    const auto lengthStr = std::to_string(http.getSize());
    this->logger.error(F("Payload from server was too big: "), lengthStr);
    return Response(NetworkStatus::BROKEN_SERVER);
  }

  // We have to simplify the errors reported by this API (but they are logged)
  const auto maybeApiStatus = this->apiStatus(rawStatus);
  if (maybeApiStatus.has_value()) {
    // The payload is always downloaded, since we check for its size and the
    // origin is trusted. If it's there it's supposed to be there.
    auto payload = http.getString();
    http.end();
    this->logger.debug(F("Payload (") , std::to_string(payload.length()), F("): "), UnsafeRawString(payload.c_str()));
    // TODO: every response occupies 2x the size because we convert String -> std::string
    return Response(iop::unwrap_ref(maybeApiStatus, IOP_CTX()), std::string(payload.c_str()));
  }
  http.end();
  return code;
}
#else
#include "driver/time.hpp"
#include "driver/wifi.hpp"

namespace iop {
void Network::setup() const noexcept {
  (void)*this;
  IOP_TRACE();
  WiFi.mode(WIFI_OFF);
  delay(1);
}
void Network::disconnect() noexcept { IOP_TRACE(); }
auto Network::isConnected() noexcept -> bool {
  IOP_TRACE();
  return true;
}
auto Network::httpRequest(const HttpMethod method,
                          const std::optional<StringView> &token, StringView path,
                          const std::optional<StringView> &data) const noexcept
    -> Result<Response, int> {
  (void)*this;
  (void)token;
  (void)method;
  (void)std::move(path);
  (void)data;
  IOP_TRACE();
  return Response(NetworkStatus::OK);
}
#endif

auto Network::httpPut(StringView token, StaticString path,
                      StringView data) const noexcept -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::PUT, std::make_optional(std::move(token)),
                           std::move(path).toStdString(),
                           std::make_optional(std::move(data)));
}

auto Network::httpPost(StringView token, const StaticString path,
                       StringView data) const noexcept
    -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, std::make_optional(std::move(token)),
                           std::move(path).toStdString(),
                           std::make_optional(std::move(data)));
}

auto Network::httpPost(StringView token, StringView path,
                       StringView data) const noexcept
    -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, std::make_optional(std::move(token)),
                           std::move(path), std::make_optional(std::move(data)));
}

auto Network::httpPost(StaticString path, StringView data) const noexcept
    -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, std::optional<StringView>(),
                           std::move(path).toStdString(),
                           std::make_optional(std::move(data)));
}

auto Network::rawStatusToString(const RawStatus &status) noexcept
    -> StaticString {
  IOP_TRACE();
  switch (status) {
  case RawStatus::CONNECTION_FAILED:
    return F("CONNECTION_FAILED");
  case RawStatus::SEND_FAILED:
    return F("SEND_FAILED");
  case RawStatus::READ_FAILED:
    return F("READ_FAILED");
  case RawStatus::ENCODING_NOT_SUPPORTED:
    return F("ENCODING_NOT_SUPPORTED");
  case RawStatus::NO_SERVER:
    return F("NO_SERVER");
  case RawStatus::READ_TIMEOUT:
    return F("READ_TIMEOUT");
  case RawStatus::CONNECTION_LOST:
    return F("CONNECTION_LOST");
  case RawStatus::OK:
    return F("OK");
  case RawStatus::SERVER_ERROR:
    return F("SERVER_ERROR");
  case RawStatus::FORBIDDEN:
    return F("FORBIDDEN");
  case RawStatus::UNKNOWN:
    return F("UNKNOWN");
  }
  return F("UNKNOWN-not-documented");
}

auto Network::rawStatus(const int code) const noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case HTTP_CODE_OK:
    return RawStatus::OK;
  case HTTP_CODE_INTERNAL_SERVER_ERROR:
    return RawStatus::SERVER_ERROR;
  case HTTP_CODE_FORBIDDEN:
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
    this->logger.warn(F("Unknown response code: "), std::to_string(code));
    return RawStatus::UNKNOWN;
  }
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

auto Network::apiStatus(const RawStatus &raw) const noexcept
    -> std::optional<NetworkStatus> {
  std::optional<NetworkStatus> ret;
  IOP_TRACE();
  switch (raw) {
  case RawStatus::CONNECTION_FAILED:
  case RawStatus::CONNECTION_LOST:
    this->logger.warn(F("Connection failed. Code: "),
                      std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case RawStatus::SEND_FAILED:
  case RawStatus::READ_FAILED:
    this->logger.warn(F("Pipe is broken. Code: "),
                      std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case RawStatus::ENCODING_NOT_SUPPORTED:
  case RawStatus::NO_SERVER:
  case RawStatus::SERVER_ERROR:
    this->logger.error(F("Server is broken. Code: "),
                       std::to_string(static_cast<int>(raw)));
    ret.emplace(NetworkStatus::BROKEN_SERVER);
    break;

  case RawStatus::READ_TIMEOUT:
    this->logger.warn(F("Network timeout triggered"));
    ret.emplace(NetworkStatus::CONNECTION_ISSUES);
    break;

  case RawStatus::OK:
    ret.emplace(NetworkStatus::OK);
    break;
  case RawStatus::FORBIDDEN:
    ret.emplace(NetworkStatus::FORBIDDEN);
    break;

  case RawStatus::UNKNOWN:
    break;
  }
  return ret;
}

Response::Response(const NetworkStatus &status) noexcept
    : status(status), payload(std::optional<std::string>()) {
  IOP_TRACE();
}
Response::Response(const NetworkStatus &status, std::string payload) noexcept
    : status(status), payload(std::move(payload)) {
  IOP_TRACE();
}
Response::Response(Response &&resp) noexcept
    : status(resp.status), payload(std::optional<std::string>()) {
  IOP_TRACE();
  this->payload = std::move(resp.payload);
}
auto Response::operator=(Response &&resp) noexcept -> Response & {
  IOP_TRACE();
  this->status = resp.status;
  this->payload = std::move(resp.payload);
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