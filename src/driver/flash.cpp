#ifdef IOP_DESKTOP
#include "driver/desktop/flash.hpp"
#else
#include "driver/esp8266/flash.hpp"
#endif

namespace driver {
    Flash flash;
}