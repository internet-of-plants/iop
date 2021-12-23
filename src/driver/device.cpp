#ifdef IOP_DESKTOP
#include "driver/desktop/device.hpp"
#else
#include "driver/esp8266/device.hpp"
#endif

namespace driver {
    Device device;
}