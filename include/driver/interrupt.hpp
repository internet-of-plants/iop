#ifndef IOP_DRIVER_INTERRUPT_HPP
#define IOP_DRIVER_INTERRUPT_HPP

#include "core/panic.hpp"

#ifdef IOP_DESKTOP
#include <pthread.h>
static pthread_mutex_t interruptLock;
#else
#include "ets_sys.h"
#endif

namespace iop {
#ifdef IOP_DESKTOP
struct InterruptLock {
  // NOLINTNEXTLINE *-use-equals-default
  InterruptLock() noexcept {
    static bool interruptInitialized = false;

    IOP_TRACE();
    // TODO: not thread safe
    if (!interruptInitialized)
      iop_assert(pthread_mutex_init(&interruptLock, NULL) != 0, F("Mutex init failed"));
    interruptInitialized = true;

    pthread_mutex_lock(&interruptLock);
  }
  // NOLINTNEXTLINE *-use-equals-default
  ~InterruptLock() noexcept {
    pthread_mutex_unlock(&interruptLock);
    IOP_TRACE();
  }
  InterruptLock(InterruptLock const &other) noexcept = default;
  InterruptLock(InterruptLock &&other) noexcept = default;
  auto operator=(InterruptLock const &other) noexcept
      -> InterruptLock & = default;
  auto operator=(InterruptLock &&other) noexcept -> InterruptLock & = default;
};
#else
struct InterruptLock {
  // NOLINTNEXTLINE *-use-equals-default
  InterruptLock() noexcept {
    IOP_TRACE();
    ETS_UART_INTR_DISABLE(); // NOLINT hicpp-signed-bitwise
  }
  // NOLINTNEXTLINE *-use-equals-default
  ~InterruptLock() noexcept {
    ETS_UART_INTR_ENABLE(); // NOLINT hicpp-signed-bitwise
    IOP_TRACE();
  }
  InterruptLock(InterruptLock const &other) noexcept = default;
  InterruptLock(InterruptLock &&other) noexcept = default;
  auto operator=(InterruptLock const &other) noexcept
      -> InterruptLock & = default;
  auto operator=(InterruptLock &&other) noexcept -> InterruptLock & = default;
};
#endif
} // namespace iop

#endif