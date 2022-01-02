#ifndef IOP_LOOP
#define IOP_LOOP
#include "api.hpp"

#include "driver/device.hpp"
#include "driver/thread.hpp"
#include "driver/network.hpp"
#include "driver/panic.hpp"
#include "configuration.hpp"
#include "flash.hpp"
#include "sensors.hpp"
#include "server.hpp"
#include "utils.hpp"

class EventLoop {
private:
  CredentialsServer credentialsServer;
  Api api_;
  iop::Log logger;
  Flash flash_;
  Sensors sensors;

  iop::esp_time nextMeasurement;
  iop::esp_time nextYieldLog;
  iop::esp_time nextNTPSync;
  iop::esp_time nextHandleConnectionLost;

  iop::esp_time nextTryFlashWifiCredentials;
  iop::esp_time nextTryHardcodedWifiCredentials;
  iop::esp_time nextTryHardcodedIopCredentials;

public:
  Api const & api() const noexcept { return this->api_; }
  Flash const & flash() const noexcept { return this->flash_; }
  void setup() noexcept;
  void loop() noexcept;
  /// Connects to WiFi
  void connect(iop::StringView ssid, iop::StringView password) const noexcept;
  
  /// Uses IoP credentials to generate an authentication token for the device
  auto authenticate(iop::StringView username, iop::StringView password, const Api &api) const noexcept -> std::optional<AuthToken>;
  auto statusToString(const driver::StationStatus status) const noexcept -> std::optional<iop::StaticString>;

private:
  void handleNotConnected() noexcept;
  void handleInterrupt(const InterruptEvent event, const std::optional<std::reference_wrapper<const AuthToken>> &maybeToken) const noexcept;
  void handleIopCredentials() noexcept;
  void handleCredentials() noexcept;
  void handleMeasurements(const AuthToken &token) noexcept;

public:
  explicit EventLoop(iop::StaticString uri, iop::LogLevel logLevel_) noexcept
      : credentialsServer(logLevel_),
        api_(std::move(uri), logLevel_),
        logger(logLevel_, IOP_STATIC_STRING("LOOP")), flash_(logLevel_),
        sensors(config::soilResistivityPower, config::soilTemperature, config::airTempAndHumidity, config::dhtVersion),
        nextMeasurement(0), nextYieldLog(0), nextNTPSync(0), nextHandleConnectionLost(0),
        nextTryFlashWifiCredentials(0), nextTryHardcodedWifiCredentials(0), nextTryHardcodedIopCredentials(0) {
    IOP_TRACE();
  }
  ~EventLoop() noexcept = default;
  auto operator=(EventLoop const &other) noexcept -> EventLoop & = default;
  auto operator=(EventLoop &&other) noexcept -> EventLoop & = default;
  EventLoop(EventLoop const &other) noexcept = default;
  EventLoop(EventLoop &&other) noexcept = default;
};

extern EventLoop eventLoop;

class Unused4KbSysStack {
  struct StackStruct {
    std::array<char, 768> text;
    AuthToken token;
    std::array<char, 64> psk;
    std::array<char, 32> ssid;
  } *data;
  
  //static_assert(sizeof(StackStruct) <= 4096);

public:
  Unused4KbSysStack() noexcept: data(new (std::nothrow) StackStruct()) {
    iop_assert(data, IOP_STATIC_STRING("Unable to allocate buffer"));
    memset((void*)data, 0, sizeof(StackStruct));
  }
  void reset() noexcept {
    memset((void*)data, 0, sizeof(StackStruct));
  }
  //Unused4KbSysStack() noexcept: data(reinterpret_cast<StackStruct *>(0x3FFFE000)) {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
  //void reset() noexcept {
  //  memset((void*)0x3FFFE000, 0, 4096);
  //}
  auto psk() noexcept -> std::array<char, 64> & {
    return this->data->psk;
  }
  auto ssid() noexcept -> std::array<char, 32> & {
    return this->data->ssid;
  }
  auto token() noexcept -> AuthToken & {
    return this->data->token;
  }
  auto text() noexcept -> std::array<char, 768> & {
    return this->data->text;
  }
  // ...
};
extern Unused4KbSysStack unused4KbSysStack;

#endif