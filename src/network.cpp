#include "network.hpp"

#include "ESP8266HTTPClient.h"

#include "generated/certificates.h"
#include "tracer.hpp"

#include "configuration.h"
#include "models.hpp"
#include "static_string.hpp"
#include "string_view.hpp"

#include "panic.hpp"
#include <memory>

#ifndef IOP_NETWORK_DISABLED

Response::Response(const ApiStatus &status) noexcept
    : status(status), payload(Option<String>()) {
  IOP_TRACE();
}
Response::Response(const ApiStatus &status, String payload) noexcept
    : status(status), payload(std::move(payload)) {
  IOP_TRACE();
}
Response::Response(Response &&resp) noexcept
    : status(resp.status), payload(Option<String>()) {
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
  if (logLevel > LogLevel::TRACE)
    return;
  Serial.print(F("~Response("));
  Serial.print(Network::apiStatusToString(status).get());
  Serial.println(F(")"));
  Serial.flush();
};

#ifdef IOP_NOSSL
static WiFiClient client;
#else
static BearSSL::CertStore certStore;     // NOLINT cert-err58-cpp
static BearSSL::WiFiClientSecure client; // NOLINT cert-err58-cpp
#endif
static HTTPClient http; // NOLINT cert-err58-cpp

auto Network::isConnected() noexcept -> bool {
  IOP_TRACE();
  return WiFi.status() == WL_CONNECTED;
}

void Network::disconnect() noexcept {
  IOP_TRACE();
  WiFi.disconnect();
}

auto Network::setup() const noexcept -> void {
  IOP_TRACE();
#ifndef IOP_NOSSL
  if (this->host().contains(F("https://"))) {
    assert_(certStoreOverrideWorked,
            F("CertStore override is broken, fix it or HTTPS won't work"));
  }
#endif

  if (!this->uri().contains(F(":"))) {
    PROGMEM_STRING(error, "Host must contain protocol (http:// or https://): ");
    panic_(String(error.get()) + F(" ") + this->uri().get());
  }

  http.setReuse(false);
  client.setNoDelay(false);
  client.setSync(true);
#ifndef IOP_NOSSL
  //  client.setCertStore(&certStore);
  client.setInsecure();
  // We should make sure our server supports Max Fragment Length Negotiation
  // if (client.probeMaxFragmentLength(uri, port, 512))
  //     client.setBufferSizes(512, 512);
#endif

#ifdef IOP_ONLINE
  // Makes sure the event handlers are never dropped and only set once
  static const auto onCallback = [](const WiFiEventStationModeGotIP &ev) {
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);
    (void)ev;
  };
  static const auto onHandler = WiFi.onStationModeGotIP(onCallback);
  // Initialize the wifi configurations

  if (Network::isConnected())
    utils::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);
  delay(1);
  WiFi.reconnect();

  // station_config status = {0};
  // wifi_station_get_config_default(&status);
  // if (status.ssid != NULL && !this->isConnected()) {
  //   WiFi.waitForConnectResult();
  // }
#else
  WiFi.mode(WIFI_OFF);
  delay(1);
#endif
}

static auto methodToString(const HttpMethod &method) noexcept
    -> Option<StaticString> {
  IOP_TRACE();
  switch (method) {
  case HttpMethod::GET:
    return StaticString(F("GET"));
  case HttpMethod::HEAD:
    return StaticString(F("HEAD"));
  case HttpMethod::POST:
    return StaticString(F("POST"));
  case HttpMethod::PUT:
    return StaticString(F("PUT"));
  case HttpMethod::PATCH:
    return StaticString(F("PATCH"));
  case HttpMethod::DELETE:
    return StaticString(F("DELETE"));
  case HttpMethod::CONNECT:
    return StaticString(F("CONNECT"));
  case HttpMethod::OPTIONS:
    return StaticString(F("OPTIONS"));
  }
  return Option<StaticString>();
}

auto Network::wifiClient() noexcept -> WiFiClient & { return client; }

// Returns Response if it can understand what the server sent, int is the raw
// status code given by ESP8266HTTPClient
auto Network::httpRequest(
    const HttpMethod method_, const Option<StringView> &token,
    const StringView path, // NOLINT performance-unnecessary-value-param
    const Option<StringView> &data) const noexcept -> Result<Response, int> {
  IOP_TRACE();

  if (!Network::isConnected())
    return Response(ApiStatus::NO_CONNECTION);

  // StringSumHelper is inferred here if we don't set the type
  const String uri = String(this->uri().get()) + path.get();

  StringView data_(emptyString);
  if (data.isSome())
    data_ = UNWRAP_REF(data);

  const auto method = UNWRAP(methodToString(method_));

  this->logger.info(F("["), method, F("] "), path,
                    data.isSome() ? F(" has payload") : F(" no payload"));
  if (token.isSome()) {
    http.setAuthorization(UNWRAP_REF(token).get());
  } else {
    http.setAuthorization(emptyString.c_str());
  }

  if (!http.begin(Network::wifiClient(), uri)) {
    this->logger.warn(F("Failed to begin http connection to "), uri);
    return Response(ApiStatus::NO_CONNECTION);
  }

  constexpr uint32_t oneMinuteMs = 60 * 1000;
  http.setTimeout(oneMinuteMs);

  if (data.isSome())
    http.addHeader(F("Content-Type"), F("application/json"));

  const auto *const data__ = reinterpret_cast<const uint8_t *>(data_.get());
  // We have to make method a String to avoid crashes

  const auto code =
      http.sendRequest(String(method.get()).c_str(), data__, data_.length());
  const auto rawStatus = this->rawStatus(code);
  const auto codeStr = std::to_string(code);
  const auto rawStatusStr = Network::rawStatusToString(rawStatus);
  this->logger.info(F("Response code ("), codeStr, F("): "), rawStatusStr);
  constexpr const int32_t maxPayloadSizeAcceptable = 2048;
  if (http.getSize() > maxPayloadSizeAcceptable) {
    http.end();
    const auto lengthStr = std::to_string(http.getSize());
    this->logger.error(F("Payload from server was too big: "), lengthStr);
    return Response(ApiStatus::BROKEN_SERVER);
  }

  const auto maybeApiStatus = this->apiStatus(rawStatus);
  if (maybeApiStatus.isSome()) {
    auto payload = http.getString();
    http.end();
    return Response(UNWRAP_REF(maybeApiStatus), std::move(payload));
  }
  http.end();
  return code;
}
#else
void Network::setup() const noexcept {
  (void)*this;
  IOP_TRACE();
}
void Network::disconnect() noexcept { IOP_TRACE(); }
auto Network::isConnected() noexcept -> bool {
  IOP_TRACE();
  return true;
}
auto Network::httpRequest(
    const HttpMethod method, const Option<StringView> &token,
    const StringView path, // NOLINT performance-unnecessary-value-param
    const Option<StringView> &data) const noexcept -> Result<Response, int> {
  (void)*this;
  (void)token;
  (void)method;
  (void)path;
  (void)data;
  IOP_TRACE();
  return Response(ApiStatus::OK);
}
#endif

auto Network::httpPut(
    const StringView token,  // NOLINT performance-unnecessary-value-param
    const StaticString path, // NOLINT performance-unnecessary-value-param
    const StringView data    // NOLINT performance-unnecessary-value-param
) const noexcept -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::PUT, token, String(path.get()), data);
}

auto Network::httpPost(
    const StringView token,  // NOLINT performance-unnecessary-value-param
    const StaticString path, // NOLINT performance-unnecessary-value-param
    const StringView data    // NOLINT performance-unnecessary-value-param
) const noexcept -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, token, String(path.get()), data);
}

auto Network::httpPost(
    const StringView token, // NOLINT performance-unnecessary-value-param
    const StringView path,  // NOLINT performance-unnecessary-value-param
    const StringView data   // NOLINT performance-unnecessary-value-param
) const noexcept -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, token, path, data);
}

auto Network::httpPost(
    const StaticString path, // NOLINT performance-unnecessary-value-param
    const StringView data    // NOLINT performance-unnecessary-value-param
) const noexcept -> Result<Response, int> {
  IOP_TRACE();
  return this->httpRequest(HttpMethod::POST, Option<StringView>(),
                           String(path.get()), data);
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
  case RawStatus::NOT_FOUND:
    return F("NOT_FOUND");
  case RawStatus::SERVER_ERROR:
    return F("SERVER_ERROR");
  case RawStatus::FORBIDDEN:
    return F("FORBIDDEN");
  case RawStatus::BAD_REQUEST:
    return F("BAD_REQUEST");
  case RawStatus::MUST_UPGRADE:
    return F("MUST_UPGRADE");
  case RawStatus::UNKNOWN:
    return F("UNKNOWN");
  }
  return F("UNKNOWN");
}

auto Network::rawStatus(const int code) const noexcept -> RawStatus {
  IOP_TRACE();
  switch (code) {
  case HTTP_CODE_OK:
    return RawStatus::OK;
  case HTTP_CODE_NOT_FOUND:
    return RawStatus::NOT_FOUND;
  case HTTP_CODE_INTERNAL_SERVER_ERROR:
    return RawStatus::SERVER_ERROR;
  case HTTP_CODE_FORBIDDEN:
    return RawStatus::FORBIDDEN;
  case HTTP_CODE_BAD_REQUEST:
    return RawStatus::BAD_REQUEST;
  case HTTP_CODE_PRECONDITION_FAILED:
    return RawStatus::MUST_UPGRADE;
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
  default:
    return RawStatus::UNKNOWN;
  }

  this->logger.warn(F("Unknown response code: "), std::to_string(code));
  return RawStatus::UNKNOWN;
}

auto Network::apiStatusToString(const ApiStatus &status) noexcept
    -> StaticString {
  IOP_TRACE();
  switch (status) {
  case ApiStatus::NO_CONNECTION:
    return F("NO_CONNECTION");
  case ApiStatus::CLIENT_BUFFER_OVERFLOW:
    return F("CLIENT_BUFFER_OVERFLOW");
  case ApiStatus::BROKEN_PIPE:
    return F("BROKEN_PIPE");
  case ApiStatus::BROKEN_SERVER:
    return F("BROKEN_SERVER");
  case ApiStatus::TIMEOUT:
    return F("TIMEOUT");
  case ApiStatus::OK:
    return F("OK");
  case ApiStatus::NOT_FOUND:
    return F("NOT_FOUND");
  case ApiStatus::FORBIDDEN:
    return F("FORBIDDEN");
  case ApiStatus::MUST_UPGRADE:
    return F("MUST_UPGRADE");
  }
  return F("UNKNOWN");
}

auto Network::apiStatus(const RawStatus &raw) const noexcept
    -> Option<ApiStatus> {
  IOP_TRACE();
  switch (raw) {
  case RawStatus::CONNECTION_FAILED:
  case RawStatus::CONNECTION_LOST:
    this->logger.warn(F("Connection failed. Code: "),
                      std::to_string(static_cast<int>(raw)));
    return ApiStatus::NO_CONNECTION;

  case RawStatus::SEND_FAILED:
  case RawStatus::READ_FAILED:
    this->logger.warn(F("Pipe is broken. Code: "),
                      std::to_string(static_cast<int>(raw)));
    return ApiStatus::BROKEN_PIPE;

  case RawStatus::ENCODING_NOT_SUPPORTED:
  case RawStatus::NO_SERVER:
  case RawStatus::SERVER_ERROR:
    this->logger.error(F("Server is broken. Code: "),
                       std::to_string(static_cast<int>(raw)));
    return ApiStatus::BROKEN_SERVER;

  case RawStatus::READ_TIMEOUT:
    this->logger.warn(F("Network timeout triggered"));
    return ApiStatus::TIMEOUT;

  case RawStatus::OK:
    return ApiStatus::OK;
  case RawStatus::NOT_FOUND:
    return ApiStatus::NOT_FOUND;
  case RawStatus::FORBIDDEN:
    return ApiStatus::FORBIDDEN;
  case RawStatus::BAD_REQUEST:
    return ApiStatus::BROKEN_SERVER;
  case RawStatus::MUST_UPGRADE:
    return ApiStatus::MUST_UPGRADE;

  case RawStatus::UNKNOWN:
    break;
  }
  return Option<ApiStatus>();
}