#ifndef IOP_LOOP
#define IOP_LOOP
#include "api.hpp"

#include "iop-hal/device.hpp"
#include "iop-hal/thread.hpp"
#include "iop-hal/network.hpp"
#include "iop-hal/panic.hpp"
#include "iop/storage.hpp"
#include "iop/server.hpp"
#include "iop/utils.hpp"

#include <functional>
#include <optional>

namespace iop {
class EventLoop;

enum class ConnectResponse {
  OK,
  TIMEOUT,
};

struct TaskInterval {
  iop::time::milliseconds next;
  uint32_t interval;
  std::function<void(EventLoop&)> func;
  TaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept;
};

struct AuthenticatedTaskInterval {
  iop::time::milliseconds next;
  uint32_t interval;
  std::function<void(EventLoop&, const AuthToken&)> func;
  AuthenticatedTaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept;
};

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger_;
  Storage storage_;

  iop::time::milliseconds nextNTPSync;

  iop::time::milliseconds nextTryStorageWifiCredentials;
  iop::time::milliseconds nextTryHardcodedWifiCredentials;
  iop::time::milliseconds nextTryHardcodedIopCredentials;

  std::vector<TaskInterval> tasks;
  std::vector<AuthenticatedTaskInterval> authenticatedTasks;
  auto syncNTP() noexcept -> void;

public:
  auto api() noexcept -> Api &{ return this->api_; }
  auto storage() noexcept -> Storage & { return this->storage_; }
  auto logger() noexcept -> Log & { return this->logger_; }
  auto setup() noexcept -> void;
  auto setAccessPointCredentials(StaticString SSID, StaticString PSK) noexcept -> void;
  auto loop() noexcept -> void;
  /// Connects to WiFi
  auto connect(std::string_view ssid, std::string_view password) noexcept -> ConnectResponse;
  
  /// Uses IoP credentials to generate an authentication token for the device
  auto authenticate(std::string_view username, std::string_view password, Api &api) noexcept -> std::unique_ptr<AuthToken>;

  auto setInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept -> void;
  auto setAuthenticatedInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, const AuthToken&)> func) noexcept -> void;
  auto registerEvent(const AuthToken& token, const Api::Json json) noexcept -> void;

private:
  auto handleNotConnected() noexcept -> void;
  auto handleStoredWifiCreds() noexcept -> void;
  auto handleHardcodedWifiCreds() noexcept -> void;

  auto handleInterrupts() noexcept -> bool;
  auto handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) noexcept -> void;

  auto handleHardcodedIopCreds() noexcept -> void;
  auto handleIopCredentials() noexcept -> void;

  auto handleCredentials() noexcept -> void;

  auto handleMeasurements(const AuthToken &token) noexcept -> void;

  auto runAuthenticatedTasks() noexcept -> void;
  auto runUnauthenticatedTasks() noexcept -> void;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(uri, logLevel_),
        logger_(logLevel_, IOP_STR("LOOP")), storage_(logLevel_),
        nextNTPSync(0), nextTryStorageWifiCredentials(0),
        nextTryHardcodedWifiCredentials(0), nextTryHardcodedIopCredentials(0) {
    IOP_TRACE();
  }
  ~EventLoop() noexcept = default;
  auto operator=(EventLoop const &other) noexcept -> EventLoop & = delete;
  auto operator=(EventLoop &&other) noexcept -> EventLoop & = default;
  EventLoop(EventLoop const &other) noexcept = delete;
  EventLoop(EventLoop &&other) noexcept = default;
};

extern auto setup(EventLoop &loop) noexcept -> void;
extern EventLoop eventLoop;
}
#endif