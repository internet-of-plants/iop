#include "driver/client.hpp"

namespace driver {
auto rawStatus(const int code) noexcept -> RawStatus {
  switch (code) {
  case 200:
    return RawStatus::OK;
  case 500:
    return RawStatus::SERVER_ERROR;
  case 403:
    return RawStatus::FORBIDDEN;

  // We generally don't use default to be able to use static-analyzers to check
  // for exaustiveness, but this is a switch on a int, so...
  default:
    return RawStatus::UNKNOWN;
  }
}

Session::~Session() noexcept {}
Session::Session(Session&& other) noexcept { (void) other; }
auto Session::operator=(Session&& other) noexcept -> Session & { (void) other; return *this; }
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept { (void) headers; (void) count; }
std::string Response::header(iop::StaticString key) const noexcept { (void) key; return ""; }
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(iop::StaticString key, iop::StringView value) noexcept { (void) key; (void) value; }
void Session::setAuthorization(std::string auth) noexcept { (void) auth; }
auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> Response { (void) method; (void) data; (void) len; return Response(500); }

HTTPClient::HTTPClient() noexcept {}
HTTPClient::~HTTPClient() noexcept {}

std::optional<Session> HTTPClient::begin(std::string uri) noexcept { (void) uri; return std::nullopt; }
}