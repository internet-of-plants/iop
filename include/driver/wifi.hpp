#ifndef IOP_DRIVER_WIFI
#define IOP_DRIVER_WIFI

#include "driver/cert_store.hpp"
#include <string>
#include <array>
#include <functional>

#ifdef IOP_ESP8266
#ifdef IOP_SSL
namespace BearSSL { class WiFiClientSecure; }
namespace driver { using NetworkClientPtr = BearSSL::WiFiClientSecure *; }
#else
class WiFiClient;
namespace driver { using NetworkClientPtr = WiFiClient *; }
#endif
#endif

namespace driver {
enum class WiFiMode {
  OFF = 0, STA, AP, AP_STA
};

enum class StationStatus {
  IDLE = 0,
  CONNECTING,
  WRONG_PASSWORD,
  NO_AP_FOUND,
  CONNECT_FAIL,
  GOT_IP
};

struct Wifi {
#ifdef IOP_ESP8266
  driver::NetworkClientPtr client;
#endif

  // TODO: document this
  
  StationStatus status() const noexcept;
  void stationDisconnect() const noexcept;
  void setMode(WiFiMode mode) const noexcept;
  WiFiMode mode() const noexcept;
  std::string localIP() const noexcept;
  void wake() const noexcept;
  std::pair<std::array<char, 32>, std::array<char, 64>> credentials() const noexcept;
  bool begin(std::string_view ssid, std::string_view psk) const noexcept;
  void setupAP() const noexcept;
  void connectAP(std::string_view ssid, std::string_view psk) const noexcept;
  std::string APIP() const noexcept;
  void reconnect() const noexcept;
  void setup(driver::CertStore *certStore) noexcept;

  void onStationModeGotIP(std::function<void()> f) noexcept;

  Wifi() noexcept;
  ~Wifi() noexcept;
  Wifi(Wifi &other) noexcept = delete;
  Wifi(Wifi &&other) noexcept;
  auto operator=(Wifi &other) noexcept -> Wifi & = delete;
  auto operator=(Wifi &&other) noexcept -> Wifi &;
};
}


#endif