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

class TaskInterval {
  iop::time::milliseconds interval;
  iop::time::milliseconds next;
  std::function<void(EventLoop&)> func;

  TaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept;
};

class AuthenticatedTaskInterval {
  iop::time::milliseconds interval;
  iop::time::milliseconds next;
  std::function<void(EventLoop&, AuthToken&)> func;

  AuthenticatedTaskInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, AuthToken&)> func) noexcept;
};

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger_;
  Storage storage_;

  iop::time::milliseconds nextNTPSync;
  iop::time::milliseconds nextHandleConnectionLost;

  iop::time::milliseconds nextTryStorageWifiCredentials;
  iop::time::milliseconds nextTryHardcodedWifiCredentials;
  iop::time::milliseconds nextTryHardcodedIopCredentials;

  std::vector<TaskInterval> tasks;
  std::vector<AuthenticatedTaskInterval> authenticatedTasks;

public:
  Api const & api() const noexcept { return this->api_; }
  Storage const & storage() const noexcept { return this->storage_; }
  auto logger() const noexcept -> iop::Log { return this->logger_; }
  void setup() noexcept;
  void loop() noexcept;
  /// Connects to WiFi
  void connect(std::string_view ssid, std::string_view password) const noexcept;
  
  /// Uses IoP credentials to generate an authentication token for the device
  auto authenticate(std::string_view username, std::string_view password, const Api &api) const noexcept -> std::optional<AuthToken>;

  auto setInterval(iop::time::milliseconds interval, std::function<void(EventLoop&)> func) noexcept -> void;
  auto setAuthenticatedInterval(iop::time::milliseconds interval, std::function<void(EventLoop&, AuthToken&)> func) noexcept -> void;

private:
  void handleNotConnected() noexcept;
  void handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &token) const noexcept;
  void handleIopCredentials() noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger_(logLevel_, IOP_STR("LOOP")), storage_(logLevel_),
        nextNTPSync(0), nextHandleConnectionLost(0), nextTryStorageWifiCredentials(0),
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