#ifndef IOP_DRIVER_WIFI
#define IOP_DRIVER_WIFI

#include <string>
#include <utility>

namespace driver {
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
  std::pair<std::string, std::string> credentials() const noexcept;
};
extern Wifi wifi;
}

#ifdef IOP_DESKTOP
#include "core/string/cow.hpp"
#include <cstring>

class IPAddress {
    private:
        uint8_t octets[4];

    public:
        // Constructors
        IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet): octets{first_octet, second_octet, third_octet, fourth_octet} {}
};

class WifiCredentials;
typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_WRONG_PASSWORD   = 6,
    WL_DISCONNECTED     = 7
} wl_status_t;
typedef enum WiFiMode 
{
    WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_AP_STA = 3,
    /* these two pseudo modes are experimental: */ WIFI_SHUTDOWN = 4, WIFI_RESUME = 8
} WiFiMode_t;

class Wifi {
  uint8_t md = WIFI_STA;
public:
  void mode(uint8_t m) { this->md = m; }
  uint8_t getMode() { return this->md; }
  void reconnect() {}
  void disconnect() {}
  void forceSleepWake() {}
  void begin(const char* ssid, const char *psk) {
      (void)ssid;
      (void)psk;
  }
  int8_t waitForConnectResult() {
      return 0;
  }
  uint8_t status() {
    return WL_CONNECTED;
  }
  void persistent(bool b) { (void)b; }
  void setAutoReconnect(bool r) { (void)r; }
  void setAutoConnect(bool c) { (void)c; }
  void softAP(const char *ssid, const char* psk) {
      (void)ssid;
      (void)psk;
  }
  void softAPConfig(IPAddress ip, IPAddress gateway, IPAddress subnet) {
    (void)ip;
    (void)gateway;
    (void)subnet;
  }

  iop::CowString localIP() {
    return iop::CowString(std::string_view("192.168.0.1"));
  }

  iop::CowString softAPIP() {
    return iop::CowString(std::string_view("127.0.0.1"));
  }
};
static Wifi WiFi;
#else
#include "ESP8266WiFi.h"
#endif

#endif