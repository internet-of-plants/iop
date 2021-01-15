#include "network.hpp"
#include "certificate_storage.hpp"
#include "generated/certificates.h"

#include "static_string.hpp"
#include "string_view.hpp"

#include "panic.hpp"
#include <memory>

#ifndef IOP_NETWORK_DISABLED
#include "ESP8266HTTPClient.h"

auto certStore = try_make_unique<BearSSL::CertStore>();
auto client = try_make_unique<WiFiClientSecure>();
auto http = try_make_unique<HTTPClient>();

void Network::disconnect() noexcept { WiFi.disconnect(); }

// TODO(pc): connections aren't working with AP active, maybe because of the DNS
// hijack of server.cpp?

// NOLINTNEXTLINE readability-convert-member-functions-to-static
auto Network::setup() const noexcept -> void {
  if (!certStore || !client || !http)
    panic_(F("Unnable to allocate CertStore, WiFiClientSecure or HTTPClient"));

#ifdef IOP_ONLINE
  // Makes sure the event handler is never dropped and only set once
  static const auto callback = [](const WiFiEventStationModeGotIP &ev) {
    interruptEvent = ON_CONNECTION;
    (void)ev;
  };
  static const auto handler = WiFi.onStationModeGotIP(callback);

  if (Network::isConnected())
    interruptEvent = ON_CONNECTION;

  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);
  WiFi.setAutoConnect(true);
  WiFi.mode(WIFI_STA);

  // station_config status = {0};
  // wifi_station_get_config_default(&status);
  // if (status.ssid != NULL || !this->isConnected()) {
  //   WiFi.waitForConnectResult();
  // }
#else
  WiFi.mode(WIFI_OFF);
#endif
}

auto Network::httpPut(const StringView token, const StaticString path,
                      const StringView data) const noexcept
    -> Result<Response, int> {
  return this->httpRequest(PUT, token, path, data);
}

auto Network::httpPost(const StringView token, const StaticString path,
                       const StringView data) const noexcept
    -> Result<Response, int> {
  return this->httpRequest(POST, token, path, data);
}

auto Network::httpPost(const StaticString path,
                       const StringView data) const noexcept
    -> Result<Response, int> {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}

static auto methodToString(const enum HttpMethod method) noexcept
    -> Option<StaticString> {
  switch (method) {
  case GET:
    return StaticString(F("GET"));
  case HEAD:
    return StaticString(F("HEAD"));
  case POST:
    return StaticString(F("POST"));
  case PUT:
    return StaticString(F("PUT"));
  case PATCH:
    return StaticString(F("PATCH"));
  case DELETE:
    return StaticString(F("DELETE"));
  case CONNECT:
    return StaticString(F("CONNECT"));
  case OPTIONS:
    return StaticString(F("OPTIONS"));
  }
  return Option<StaticString>();
}

auto Network::wifiClient(const StaticString path) const noexcept
    -> Result<std::reference_wrapper<WiFiClientSecure>, RawStatus> {
  if (!this->host_.contains(F(":"))) {
    PROGMEM_STRING(error, "Host must contain protocol (http:// or https://): ");
    panic_(String(error.get()) + this->host_.asCharPtr());
  }
  const auto uri = String(this->host_.get()) + path.get();
  this->logger.info(uri);
  constexpr const auto port = 4001;

  if (Network::isConnected()) {
    // We should make sure our server supports Max Fragment Length Negotiation
    // if (client->probeMaxFragmentLength(uri, port, 512)) {
    //   client->setBufferSizes(512, 512);
    // }
    client->setNoDelay(false);
    client->setSync(true);
    client->setCertStore(certStore.get());
    // client->setInsecure();
    if (client->connect(uri, port) <= 0) {
      this->logger.warn(F("Failed to connect to "), uri);
      return RawStatus::CONNECTION_FAILED;
    }
    client->disableKeepAlive();
  } else {
    return RawStatus::CONNECTION_LOST;
  }
  return std::ref(*client);
}

// Returns Response if it can understand what the server sent, int is the raw
// status code given by ESP8266HTTPClient
auto Network::httpRequest(const enum HttpMethod method_,
                          const Option<StringView> token,
                          const StaticString path,
                          const Option<StringView> data) const noexcept
    -> Result<Response, int> {
  const auto clientResult = this->wifiClient(path);
  if (IS_ERR(clientResult)) {
    const auto rawStatus = UNWRAP_ERR_REF(clientResult);
    const auto apiStatus = this->apiStatus(rawStatus);
    if (apiStatus.isSome())
      return Response(UNWRAP_REF(apiStatus));

    const auto s = Network::rawStatusToString(rawStatus);
    this->logger.warn(F("Network::httpClient returned invalid RawStatus: "), s);
    return static_cast<int>(rawStatus);
  }

  StringView data_(StaticString(F("")));
  if (data.isSome()) {
    {
      { data_ = UNWRAP_REF(data); }
    }
  }

  const auto method = UNWRAP(methodToString(method_));

  this->logger.info(F("["), method, F("] "), path);
  // TODO(pc): will this be a problem if we start streaming logs?
  // credentials logged are a big no-no
  this->logger.debug(F("Json data: "), data_);

  const auto uri = String(this->host_.get()) + path.get();
  if (!http->begin(*client, uri)) {
    this->logger.warn(F("Failed to begin http connection to "), uri);
    return Response(ApiStatus::NO_CONNECTION);
  }
  http->setReuse(false);

  if (token.isSome()) {
    {
      { http->setAuthorization(UNWRAP_REF(token).get()); }
    }
  }

  if (data.isSome()) {
    {
      { http->addHeader(F("Content-Type"), F("application/json")); }
    }
  }

  const auto *const data__ = reinterpret_cast<const uint8_t *>(data_.get());
  const auto *const mtd = method.asCharPtr();
  const auto code = http->sendRequest(mtd, data__, data_.length());
  const auto codeStr = std::to_string(code);

  const auto rawStatus = this->rawStatus(code);
  const auto rawStatusStr = Network::rawStatusToString(rawStatus);

  this->logger.info(F("Response code ("), codeStr, F("): "), rawStatusStr);

  constexpr const int32_t maxPayloadSizeAcceptable = 2048;
  if (http->getSize() > maxPayloadSizeAcceptable) {
    const auto lengthStr = std::to_string(http->getSize());
    this->logger.error(F("Payload from server was too big: "), lengthStr);
    return Response(ApiStatus::BROKEN_SERVER);
  }

  const auto maybeApiStatus = this->apiStatus(rawStatus);
  if (maybeApiStatus.isSome()) {
    return Response(UNWRAP_REF(maybeApiStatus), http->getString());
  }
  return code;
}
#endif

auto Network::rawStatusToString(const RawStatus status) noexcept
    -> StaticString {
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

auto Network::apiStatusToString(const ApiStatus status) noexcept
    -> StaticString {
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

auto Network::apiStatus(const RawStatus raw) const noexcept
    -> Option<ApiStatus> {
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
    this->logger.warn(F("Server is broken. Code: "),
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

#ifdef IOP_NETWORK_DISABLED
void Network::setup() const noexcept {}
Option<Response> Network::httpPut(const StringView token,
                                  const StaticString path,
                                  const StringView data) const noexcept {
  return this->httpRequest(PUT, token, path, data);
}
Option<Response> Network::httpPost(const StringView token,
                                   const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, token, path, data);
}
Option<Response> Network::httpPost(const StaticString path,
                                   const StringView data) const noexcept {
  return this->httpRequest(POST, Option<StringView>(), path, data);
}
Option<std::shared_ptr<HTTPClient>>
httpClient(const StaticString path,
           const Option<StringView> &token) const noexcept {
  (void)path;
  (void)token;
  return http;
}
Option<Response> Network::httpRequest(const enum HttpMethod method,
                                      const Option<StringView> token,
                                      const StaticString path,
                                      Option<StringView> data) const noexcept {
  (void)method;
  (void)token;
  (void)path;
  (void)data;
  return Response(ApiStatus::OK, F(""));
}
void Network::disconnect() const noexcept {}
#endif