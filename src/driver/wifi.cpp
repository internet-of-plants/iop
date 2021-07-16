#include "driver/wifi.hpp"

namespace driver {
Wifi wifi;
}
#ifdef IOP_DESKTOP
namespace driver {
StationStatus Wifi::status() const noexcept {
    IOP_TRACE()
    return StationStatus::IDLE;
}
void Wifi::stationDisconnect() const noexcept {}
std::pair<std::string, std::string> Wifi::credentials() const noexcept {
  IOP_TRACE()
  return std::make_pair("SSID", "PSK");
}
}
#else
#include <string>
#include "core/panic.hpp"
#include "driver/interrupt.hpp"

namespace driver {
StationStatus Wifi::status() const noexcept {
    IOP_TRACE()
    const auto s = wifi_station_get_connect_status();
    switch (s) {
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
    }
    iop_panic(iop::StaticString(F("Unreachable status: ")).toString() + std::to_string(static_cast<uint8_t>(s)));
}
void Wifi::stationDisconnect() const noexcept {
    IOP_TRACE()
    const iop::InterruptLock _guard;
    wifi_station_disconnect();
}
std::pair<std::string, std::string> Wifi::credentials() const noexcept {
    IOP_TRACE()

    station_config config;
    memset(&config, '\0', sizeof(config));
    wifi_station_get_config(&config);

    auto ssid = std::string(sizeof(config.ssid), '\0');
    std::memcpy(ssid.data(), config.ssid, sizeof(config.ssid));

    auto psk = std::string(sizeof(config.password), '\0');
    std::memcpy(psk.data(), config.password, sizeof(config.password));
    return std::make_pair(ssid, psk);
}
}
#endif