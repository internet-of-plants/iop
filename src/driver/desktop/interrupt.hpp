#include "driver/interrupt.hpp"
#include "driver/panic.hpp"
#include <mutex>

static std::mutex interruptMutex;

namespace driver {
InterruptLock::InterruptLock() noexcept {
  interruptMutex.lock();
}

InterruptLock::~InterruptLock() noexcept {
  interruptMutex.unlock();
  IOP_TRACE();
}
}