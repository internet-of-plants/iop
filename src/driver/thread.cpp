#ifdef IOP_DESKTOP
#include "driver/desktop/thread.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/thread.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/thread.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
    Thread thisThread;
}