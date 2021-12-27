#include "driver/client.hpp"

namespace driver {
Session::~Session() noexcept {}
Session::Session(Session&& other) noexcept {}
auto Session::operator=(Session&& other) noexcept -> Session & { return *this; }
void HTTPClient::headersToCollect(const char * headers[], size_t count) noexcept { (void) headers; (void) count; }
std::string Response::header(iop::StaticString key) const noexcept { return ""; }
void Session::addHeader(iop::StaticString key, iop::StaticString value) noexcept { (void) key; (void) value; }
void Session::addHeader(iop::StaticString key, std::string_view value) noexcept { (void) key; (void) value; }
void Session::setAuthorization(std::string auth) noexcept { (void) auth; }
auto Session::sendRequest(std::string method, const uint8_t *data, size_t len) noexcept -> std::variant<Response, int> { (void) method; (void) data; (void) len; return 500; }

HTTPClient::HTTPClient() noexcept {}
HTTPClient::~HTTPClient() noexcept {}

std::optional<Session> HTTPClient::begin(std::string uri) noexcept { (void) uri; return std::nullopt; }
}