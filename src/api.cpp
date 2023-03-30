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
static void updateScheduler() noexcept {
  iop::scheduleInterrupt(iop::InterruptEvent::MUST_UPGRADE);
}

auto Api::setup() const noexcept -> void {
  IOP_TRACE();

  // Sets scheduler for update interrupt
  iop::Network::setUpdateHook(iop::UpdateHook(updateScheduler));

  this->network.setup();
}

auto Api::reportPanic(const AuthToken &authToken, const PanicData &event) noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.info(IOP_STR("Report iop_panic: "));
  this->logger.infoln(event.msg);

  auto msg = event.msg;
  auto json = Api::Json();

  while (true) {
    const auto make = [event, &msg](JsonDocument &doc) {
      doc["file"] = event.file.toString();
      doc["line"] = event.line;
      doc["func"] = event.func.toString();
      doc["msg"] = msg;
    };
    json = this->makeJson(IOP_FUNC, make);

    if (!json) {
      if (msg.length() == 0) {
        return iop::NetworkStatus::BROKEN_CLIENT;
      }

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
  if (!status || *status == iop::NetworkStatus::IO_ERROR) {
    this->logger.error(IOP_STR("Unexpected response at Api::reportPanic: "));
    this->logger.errorln(response.code());
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}

auto Api::registerEvent(const AuthToken &authToken, const Api::Json &event) noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  this->logger.infoln(IOP_STR("Send event"));

  const auto token = iop::to_view(authToken);
  const auto response = this->network.httpPost(token, IOP_STR("/v1/event"), iop::to_view(event));

  const auto status = response.status();
  if (!status || *status == iop::NetworkStatus::IO_ERROR) {
    this->logger.error(IOP_STR("Unexpected response at Api::registerEvent: "));
    this->logger.errorln(response.code());
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}
auto Api::authenticate(std::string_view username, std::string_view password) noexcept -> std::variant<std::unique_ptr<AuthToken>, iop::NetworkStatus> {
  IOP_TRACE();

  this->logger.info(IOP_STR("Authenticate IoP user: "));
  this->logger.infoln(username);

  if (!username.length() || !password.length()) {
    this->logger.warnln(IOP_STR("Empty username or password, at Api::authenticate"));
    return iop::NetworkStatus::UNAUTHORIZED;
  }

  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = username;
    doc["password"] = password;
  };

  const auto json = this->makeJson(IOP_FUNC, make);
  if (!json) {
    return iop::NetworkStatus::BROKEN_CLIENT;
  }

  auto response = this->network.httpPost(IOP_STR("/v1/user/login"), iop::to_view(*json));

  const auto status = response.status();
  if (!status || *status == iop::NetworkStatus::IO_ERROR) {
    this->logger.error(IOP_STR("Unexpected response at Api::authenticate: "));
    this->logger.errorln(response.code());
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  const auto payloadBuff = std::move(response.await().payload);
  const auto payload = iop::to_view(payloadBuff);

  if (*status != iop::NetworkStatus::OK) {
    this->logger.error(IOP_STR("Status code: "));
    this->logger.errorln(response.code());
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  if (payload.length() == 0) {
    this->logger.errorln(IOP_STR("Server answered OK, but payload is missing"));
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  if (!iop::isAllPrintable(payload)) {
    this->logger.error(IOP_STR("Unprintable payload, this isn't supported: "));
    this->logger.errorln(iop::scapeNonPrintable(payload));
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  if (payload.length() != 64) {
    this->logger.error(IOP_STR("Auth token does not occupy 64 bytes: size = "));
    this->logger.errorln(payload.length());
    return iop::NetworkStatus::BROKEN_SERVER;
  }

  auto token = std::unique_ptr<AuthToken>(new (std::nothrow) AuthToken());
  if (!token) {
    this->logger.errorln(IOP_STR("Unable to allocate auth token"));
    return iop::NetworkStatus::BROKEN_CLIENT;
  }
  memcpy(token->data(), payload.begin(), payload.length());
  return token;
}

auto Api::registerLog(const AuthToken &authToken, std::string_view log) noexcept -> iop::NetworkStatus {
  IOP_TRACE();
  const auto token = iop::to_view(authToken);
  this->logger.debug(IOP_STR("Register logToken: "));
  this->logger.debugln(token);
  this->logger.debug(IOP_STR("Log: "));
  this->logger.debugln(log);
  auto const response = this->network.httpPost(token, IOP_STR("/v1/log"), log);

  const auto status = response.status();
  if (!status || *status == iop::NetworkStatus::IO_ERROR) {
    this->logger.error(IOP_STR("Unexpected response at Api::registerLog: "));
    this->logger.errorln(response.code());
    return iop::NetworkStatus::BROKEN_SERVER;
  }
  return *status;
}

auto Api::update(const AuthToken &token) noexcept
    -> iop_hal::UpdateStatus {
  IOP_TRACE();
  this->logger.infoln(IOP_STR("Upgrading sketch"));

  return this->network.update(IOP_STR("/v1/update"), iop::to_view(token));
}

Api::Api(iop::StaticString uri) noexcept
    : network(uri), logger(IOP_STR("API")) {
  IOP_TRACE();
}

using FixedJsonBuffer = StaticJsonDocument<Api::JsonCapacity>;

auto Api::makeJson(const iop::StaticString contextName, const Api::JsonCallback jsonObjectBuilder) noexcept -> Api::Json {
  IOP_TRACE();

  auto doc = std::unique_ptr<FixedJsonBuffer>(new (std::nothrow) FixedJsonBuffer());
  if (!doc) return nullptr;
  doc->clear(); // Zeroes heap memory
  jsonObjectBuilder(*doc);

  if (doc->overflowed()) {
    this->logger.error(IOP_STR("Payload doesn't fit buffer at "));
    this->logger.errorln(contextName);
    return nullptr;
  }

  auto json = std::unique_ptr<std::array<char, JsonCapacity>>(new (std::nothrow) std::array<char, JsonCapacity>());
  if (!json) return nullptr;
  json->fill('\0'); // Zeroes heap memory
  serializeJson(*doc, json->data(), json->size());

  return json;
}
}