#ifdef IOP_DESKTOP
#include "driver/desktop/wifi.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/wifi.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/wifi.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/wifi.hpp"
#else
#error "Target not supported"
#endif