#ifndef IOP_DRIVER_WIFI
#define IOP_DRIVER_WIFI

#include <string>

#ifndef IOP_DESKTOP
#include "ESP8266WiFi.h"
#endif

#include "core/cert_store.hpp"

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


class Wifi {
public:
  #ifndef IOP_DESKTOP
  #ifdef IOP_SSL
  BearSSL::WiFiClientSecure client;
  #else
  WiFiClient client;
  #endif
  #endif
  
  Wifi() = default;
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
  void setup(iop::CertStore *certStore) noexcept;
};
}


#endif