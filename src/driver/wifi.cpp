#include "driver/wifi.hpp"

#ifdef IOP_DESKTOP
#include "core/log.hpp"
namespace driver {
StationStatus Wifi::status() const noexcept {
    return StationStatus::GOT_IP;
}
void Wifi::stationDisconnect() const noexcept {}
std::pair<std::array<char, 32>, std::array<char, 64>> Wifi::credentials() const noexcept {
  IOP_TRACE()
  std::array<char, 32> ssid;
  ssid.fill('\0');
  memcpy(ssid.data(), "SSID", 4);
  
  std::array<char, 64> psk;
  psk.fill('\0');
  memcpy(psk.data(), "PSK", 3);
  
  return std::make_pair(ssid, psk);
}
std::string Wifi::localIP() const noexcept {
    return "127.0.0.1";
}
std::string Wifi::APIP() const noexcept {
    return "127.0.0.1";
}
void Wifi::connectAP(std::string_view ssid, std::string_view psk) const noexcept {
    (void) ssid;
    (void) psk;
}
bool Wifi::begin(std::string_view ssid, std::string_view psk) const noexcept {
    (void) ssid;
    (void) psk;
    return true;
}
WiFiMode Wifi::mode() const noexcept {
    return WiFiMode::AP_STA;
}
void Wifi::wake() const noexcept {}
void Wifi::setupAP() const noexcept {}
void Wifi::reconnect() const noexcept {}
void Wifi::setup(iop::CertStore *certStore) noexcept { (void)certStore; }
void Wifi::setMode(WiFiMode mode) const noexcept { (void) mode; }
}
#else
#include <string>
#include "core/panic.hpp"
#include "driver/interrupt.hpp"
#include "ESP8266WiFi.h"

namespace driver {
StationStatus Wifi::status() const noexcept {
    const auto s = wifi_station_get_connect_status();
    switch (static_cast<int>(s)) {
        case STATION_IDLE:
            return StationStatus::IDLE;
        case STATION_CONNECTING:
            return StationStatus::CONNECTING;
        case STATION_WRONG_PASSWORD:
            return StationStatus::WRONG_PASSWORD;
        case STATION_NO_AP_FOUND:
            return StationStatus::NO_AP_FOUND;
        case STATION_CONNECT_FAIL:
            return StationStatus::CONNECT_FAIL;
        case STATION_GOT_IP:
            return StationStatus::GOT_IP;
        case 255: // No idea what this is, but it's returned sometimes;
            return StationStatus::IDLE;
    }
    iop_panic(iop::StaticString(F("Unreachable status: ")).toString() + std::to_string(static_cast<uint8_t>(s)));
}
void Wifi::setupAP() const noexcept {
    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto staticIp = IPAddress(192, 168, 1, 1);
    // NOLINTNEXTLINE *-avoid-magic-numbers
    const auto mask = IPAddress(255, 255, 255, 0);
    ::WiFi.softAPConfig(staticIp, staticIp, mask);
}
std::string Wifi::APIP() const noexcept {
    return ::WiFi.softAPIP().toString().c_str();
}
void Wifi::setMode(WiFiMode mode) const noexcept {
    switch (mode) {
        case WiFiMode::OFF:
            ::WiFi.mode(WIFI_OFF);
            return;
        case WiFiMode::STA:
            ::WiFi.mode(WIFI_STA);
            return;
        case WiFiMode::AP:
            ::WiFi.mode(WIFI_AP);
            return;
        case WiFiMode::AP_STA:
            ::WiFi.mode(WIFI_AP_STA);
            return;
    }
    iop_panic(F("Unreachable"));
}
void Wifi::reconnect() const noexcept {
    ::WiFi.reconnect();
    ::WiFi.waitForConnectResult();
}
std::string Wifi::localIP() const noexcept {
    return ::WiFi.localIP().toString().c_str();
}
void Wifi::stationDisconnect() const noexcept {
    IOP_TRACE()
    const iop::InterruptLock _guard;
    wifi_station_disconnect();
}
std::pair<std::array<char, 32>, std::array<char, 64>> Wifi::credentials() const noexcept {
    IOP_TRACE()

    station_config config;
    memset(&config, '\0', sizeof(config));
    wifi_station_get_config(&config);

    auto ssid = std::array<char, 32>();
    ssid.fill('\0');
    std::memcpy(ssid.data(), config.ssid, sizeof(config.ssid));

    auto psk = std::array<char, 64>();
    psk.fill('\0');
    std::memcpy(psk.data(), config.password, sizeof(config.password));

    return std::make_pair(ssid, psk);
}
void Wifi::wake() const noexcept {
    ::WiFi.forceSleepWake();
}
void Wifi::setup(iop::CertStore *certStore) noexcept {
  #ifdef IOP_SSL
  iop_assert(certStore != nullptr, F("CertStore is not set, but SSL is enabled"));
  this->client.setCertStore(certStore);
  #endif
  ::WiFi.persistent(false);
  ::WiFi.setAutoReconnect(false);
  ::WiFi.setAutoConnect(false);
}
WiFiMode Wifi::mode() const noexcept {
    switch (::WiFi.getMode()) {
        case WIFI_OFF:
            return WiFiMode::OFF;
        case WIFI_STA:
            return WiFiMode::STA;
        case WIFI_AP:
            return WiFiMode::AP;
        case WIFI_AP_STA:
            return WiFiMode::AP_STA;
    }
    iop_panic(F("Unreachable"));
}
void Wifi::connectAP(std::string_view ssid, std::string_view psk) const noexcept {
    String ssidStr;
    ssidStr.concat(ssid.begin(), ssid.length());
    String pskStr;
    pskStr.concat(psk.begin(), psk.length());
    ::WiFi.softAP(ssidStr, pskStr);
}
bool Wifi::begin(std::string_view ssid, std::string_view psk) const noexcept {
    String ssidStr;
    ssidStr.concat(ssid.begin(), ssid.length());
    String pskStr;
    pskStr.concat(psk.begin(), psk.length());
    ::WiFi.begin(ssidStr, pskStr);

    return ::WiFi.waitForConnectResult() != -1;
}
}
#endif