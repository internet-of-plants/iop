#ifndef IOP_DRIVER_WIFI
#define IOP_DRIVER_WIFI

#include <string>

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
  StationStatus status() const noexcept;
  void stationDisconnect() const noexcept;
  void setMode(WiFiMode mode) const noexcept;
  WiFiMode mode() const noexcept;
  std::string localIP() const noexcept;
  void wake() const noexcept;
  std::pair<std::string, std::string> credentials() const noexcept;
  bool begin(std::string_view ssid, std::string_view psk) const noexcept;
  void setupAP() const noexcept;
  void connectAP(std::string_view ssid, std::string_view psk) const noexcept;
  std::string APIP() const noexcept;
  void reconnect() const noexcept;
  void setup() const noexcept;
};
extern Wifi wifi;
}


#endif