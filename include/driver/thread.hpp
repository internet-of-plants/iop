#ifndef IOP_DRIVER_THREAD
#define IOP_DRIVER_THREAD

#include <stdint.h>

namespace iop {
using esp_time = unsigned long; // NOLINT google-runtime-int
}

namespace driver {
class Thread {
public:
  auto now() const noexcept -> iop::esp_time;
  void sleep(uint64_t ms) const noexcept;
  void yield() const noexcept;
  void panic_() const noexcept __attribute__((noreturn));
};

extern Thread thisThread;
}

#endif