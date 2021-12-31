#include "driver/wifi.hpp"
#include "driver/interrupt.hpp"
#include "driver/esp32/internal_cert_store.hpp"
#include "driver/panic.hpp"
#include "ESP8266WiFi.h"

namespace driver { 
Wifi::Wifi() noexcept: client(new (std::nothrow) std::remove_pointer<driver::NetworkClientPtr>::type()) {
    iop_assert(client, IOP_STATIC_STRING("OOM"));
}

Wifi::~Wifi() noexcept {
    delete this->client;
}

void Wifi::onStationModeGotIP(std::function<void()> f) noexcept {
  ::WiFi.onStationModeGotIP([f](const ::WiFiEventStationModeGotIP &ev) {
      (void) ev;
      f();
  });
}

Wifi::Wifi(Wifi &&other) noexcept: client(other.client) {
    other.client = nullptr;
}

auto Wifi::operator=(Wifi &&other) noexcept -> Wifi & {
    this->client = other.client;
    other.client = nullptr;
    return *this;
}

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
    iop_panic(IOP_STATIC_STRING("Unreachable status: ").toString() + std::to_string(static_cast<uint8_t>(s)));
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
    iop_panic(IOP_STATIC_STRING("Unreachable"));
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
    const driver::InterruptLock _guard;
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

void Wifi::setup(driver::CertStore *certStore) noexcept {
  iop_assert(this->client, IOP_STATIC_STRING("Wifi has been moved out, client is nullptr"));

  #ifdef IOP_SSL
  iop_assert(certStore && certStore->internal, IOP_STATIC_STRING("CertStore is not set, but SSL is enabled"));
  this->client->setCertStore(certStore->internal);
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
    iop_panic(IOP_STATIC_STRING("Unreachable"));
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