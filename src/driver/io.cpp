#ifdef IOP_DESKTOP
#include "driver/desktop/io.hpp"
#else
#include "driver/esp8266/io.hpp"
#endif

namespace driver {
driver::io::GPIO gpio;
}
