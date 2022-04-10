#include "iop-hal/client.hpp"
#include "iop-hal/server.hpp"
#include "iop-hal/panic.hpp"
#include "iop/api.hpp"
#include "iop/utils.hpp"
#include "iop/loop.hpp"
#include <string>

// TODO: have an endpoint to report non 200 response
// TODO: have an endpoint to report BROKEN_CLIENTS

namespace iop {
static void upgradeScheduler() noexcept {
  iop::scheduleInterrupt(iop::InterruptEvent::MUST_UPGRADE);
}
void onWifiConnect() noexcept {
  iop::scheduleInterrupt(iop::InterruptEvent::ON_CONNECTION);
}

auto Api::setup() const noexcept -> void {
  IOP_TRACE();

  iop::wifi.onConnect(onWifiConnect);
  // If we are already connected the callback won't be called
  if (iop::Network::isConnected())
    iop::scheduleInterrupt(InterruptEvent::ON_CONNECTION);

  // Sets scheduler for upgrade interrupt
  iop::Network::setUpgradeHook(iop::UpgradeHook(upgradeScheduler));

  this->network.setup();
}

auto Api::reportPanic(const AuthToken &authToken, const PanicData &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Report iop_panic: "), event.msg);

  auto msg = event.msg;
  auto json = std::unique_ptr<Api::Json>();

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    json = this->makeJson(IOP_FUNC, make);

    if (!json) {
      iop_assert(msg.length() / 2 != 0, IOP_STR("Message would be empty, function is broken"));
      msg = msg.substr(0, msg.length() / 2);
      continue;
    }
    break;
  }

  if (!json)
    return iop::NetworkStatus::BROKEN_CLIENT;

  const auto token = iop::to_view(authToken);
  const auto response = this->network.httpPost(token, IOP_STR("/v1/panic"), iop::to_view(*json));

  const auto status = response.status();
  if (!status) {
    this->logger.error(IOP_STR("Unexpected response at Api::reportPanic: "), std::to_string(response.code()));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}

auto Api::registerEvent(const AuthToken &authToken, const Api::Json &event) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Send event"));

  const auto token = iop::to_view(authToken);
  const auto response = this->network.httpPost(token, IOP_STR("/v1/event"), iop::to_view(event));

  const auto status = response.status();
  if (!status) {
    this->logger.error(IOP_STR("Unexpected response at Api::registerEvent: "), std::to_string(response.code()));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}
auto Api::authenticate(std::string_view username, std::string_view password) const noexcept -> std::variant<AuthToken, iop::NetworkStatus> {
  IOP_TRACE();
  this->logger.info(IOP_STR("Authenticate IoP user: "), username);

  if (!username.length() || !password.length()) {
    this->logger.warn(IOP_STR("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::FORBIDDEN;
  }

  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = username;
    doc["password"] = password;
  };
  const auto json = this->makeJson(IOP_FUNC, make);

  if (!json)
    return iop::NetworkStatus::BROKEN_CLIENT;
  auto response = this->network.httpPost(IOP_STR("/v1/user/login"), iop::to_view(*json));

  const auto status = response.status();
  if (!status) {
    this->logger.error(IOP_STR("Unexpected response at Api::authenticate: "), std::to_string(response.code()));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto payloadBuff = std::move(response.await().payload);
  const auto payload = iop::to_view(payloadBuff);

  if (payload.length() == 0) {
    this->logger.error(IOP_STR("Server answered OK, but payload is missing"));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  if (!iop::isAllPrintable(payload)) {
    this->logger.error(IOP_STR("Unprintable payload, this isn't supported: "), iop::to_view(iop::scapeNonPrintable(payload)));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  if (payload.length() != 64) {
    this->logger.error(IOP_STR("Auth token does not occupy 64 bytes: size = "), std::to_string(payload.length()));
  }

  AuthToken token;
  memcpy(token.data(), payload.begin(), payload.length());
  return token;
}

auto Api::registerLog(const AuthToken &authToken, std::string_view log) const noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.info(IOP_STR("Register log. Token: "), token, IOP_STR(". Log: "), log);
  auto const response = this->network.httpPost(token, IOP_STR("/v1/log"), log);

  const auto status = response.status();
  if (!status) {
    this->logger.error(IOP_STR("Unexpected response at Api::registerLog: "), std::to_string(response.code()));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}

auto Api::upgrade(const AuthToken &token) const noexcept
    -> iop_hal::UpgradeStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Upgrading sketch"));

  return this->network.upgrade(IOP_STR("/v1/update"), iop::to_view(token));
}

Api::Api(iop::StaticString uri, const iop::LogLevel logLevel) noexcept
    : network(std::move(uri), logLevel), logger(logLevel, IOP_STR("API")) {
  IOP_TRACE();
}

using FixedJsonBuffer = StaticJsonDocument<Api::JsonCapacity>;

auto Api::makeJson(const iop::StaticString contextName, const Api::JsonCallback jsonObjectBuilder) const noexcept -> std::unique_ptr<Api::Json> {
  IOP_TRACE();
  
  auto doc = std::unique_ptr<FixedJsonBuffer>(new (std::nothrow) FixedJsonBuffer());
  if (!doc) return nullptr;
  doc->clear(); // Zeroes heap memory
  jsonObjectBuilder(*doc);

  if (doc->overflowed()) {
    this->logger.error(IOP_STR("Payload doesn't fit buffer at "), contextName);
    return nullptr;
  }

  auto json = std::unique_ptr<Api::Json>(new (std::nothrow) Api::Json());
  if (!json) return nullptr;
  json->fill('\0'); // Zeroes heap memory
  serializeJson(*doc, json->data(), json->size());
  
  // This might leak sensitive information
  this->logger.debug(IOP_STR("Json: "), iop::to_view(*json));
  return json;
}
}