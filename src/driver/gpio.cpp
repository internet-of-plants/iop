#ifdef IOP_DESKTOP
#include "driver/desktop/gpio.hpp"
#else
#include "driver/esp8266/gpio.hpp"
#endif

namespace gpio {
    gpio::GPIO gpio;
}
