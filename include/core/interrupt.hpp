#ifndef IOP_CORE_INTERRUPT_HPP
#define IOP_CORE_INTERRUPT_HPP

#ifdef IOP_DESKTOP
#define ETS_UART_INTR_DISABLE()
#define ETS_UART_INTR_ENABLE()
#else
#include "ets_sys.h"
#endif

namespace iop {
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
} // namespace iop

#endif