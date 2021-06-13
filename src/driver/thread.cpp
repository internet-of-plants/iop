#include "driver/thread.hpp"

namespace driver {
    Thread thisThread;
}

#ifdef IOP_DESKTOP
#include <chrono>
#include <thread>

namespace driver {
void Thread::sleep(uint64_t ms) const noexcept {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
void Thread::yield() const noexcept {
  std::this_thread::yield();
}
void Thread::panic_() const noexcept {
  std::abort();
}
static const auto start = std::chrono::system_clock::now().time_since_epoch();
auto Thread::now() const noexcept -> iop::esp_time {
    return std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() - start).time_since_epoch()).count();
}
}
#else
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
}
#endif