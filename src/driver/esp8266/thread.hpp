#include "driver/thread.hpp"
#include "Arduino.h"

namespace driver {
void Thread::sleep(uint64_t ms) const noexcept {
  ::delay(ms);
}
void Thread::yield() const noexcept {
  ::yield();
}
void Thread::panic_() const noexcept {
    __panic_func(__FILE__, __LINE__, __PRETTY_FUNCTION__);
}
auto Thread::now() const noexcept -> iop::esp_time {
    return millis();
}

Thread thisThread;
}