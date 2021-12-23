#include "driver/interrupt.hpp"
#include "core/log.hpp"
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