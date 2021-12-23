#include "driver/interrupt.hpp"
#include "driver/panic.hpp"
#include <pthread.h>

static pthread_mutex_t interruptLock;

namespace iop {
InterruptLock::InterruptLock() noexcept {
  static bool interruptInitialized = false;

  IOP_TRACE();
  if (!interruptInitialized)
    iop_assert(pthread_mutex_init(&interruptLock, NULL) != 0, FLASH("Mutex init failed"));
  interruptInitialized = true;

  pthread_mutex_lock(&interruptLock);
}

InterruptLock::~InterruptLock() noexcept {
  pthread_mutex_unlock(&interruptLock);
  IOP_TRACE();
}
}