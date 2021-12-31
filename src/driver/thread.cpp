#ifdef IOP_DESKTOP
#include "driver/desktop/thread.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/thread.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/thread.hpp"
#elif defined(IOP_ESP32)
#include "driver/esp32/thread.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
    Thread thisThread;
}