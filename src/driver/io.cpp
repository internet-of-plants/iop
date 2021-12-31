#ifdef IOP_DESKTOP
#include "driver/desktop/io.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/io.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/io.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/io.hpp"
#else
#error "Target not supported"
#endif

#include "driver/io.hpp"

namespace driver {
driver::io::GPIO gpio;
}
