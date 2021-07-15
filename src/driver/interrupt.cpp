#include "driver/interrupt.hpp"
#include "core/log.hpp"
#ifdef IOP_DESKTOP
#include "core/panic.hpp"
#include <pthread.h>
static pthread_mutex_t interruptLock;

namespace iop {
InterruptLock::InterruptLock() noexcept {
  static bool interruptInitialized = false;

  IOP_TRACE();
  if (!interruptInitialized)
    iop_assert(pthread_mutex_init(&interruptLock, NULL) != 0, F("Mutex init failed"));
  interruptInitialized = true;

  pthread_mutex_lock(&interruptLock);
}

InterruptLock::~InterruptLock() noexcept {
  pthread_mutex_unlock(&interruptLock);
  IOP_TRACE();
}
}
#else
#include "ets_sys.h"

namespace iop {
InterruptLock::InterruptLock() noexcept {
  IOP_TRACE();
  ETS_UART_INTR_DISABLE(); // NOLINT hicpp-signed-bitwise
}
InterruptLock::~InterruptLock() noexcept {
  ETS_UART_INTR_ENABLE(); // NOLINT hicpp-signed-bitwise
  IOP_TRACE();
}
}
#endif