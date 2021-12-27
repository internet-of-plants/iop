#include "driver/thread.hpp"

namespace driver {
void Thread::sleep(uint64_t ms) const noexcept { (void) ms; }
void Thread::yield() const noexcept {}
void Thread::panic_() const noexcept { while (true) {} }
auto Thread::now() const noexcept -> iop::esp_time { return 0; }
}