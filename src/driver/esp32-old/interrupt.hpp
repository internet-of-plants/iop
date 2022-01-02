#include "driver/interrupt.hpp"
#include "driver/log.hpp"
#include "ets_sys.h"

namespace driver {
InterruptLock::InterruptLock() noexcept {
  IOP_TRACE();
  ETS_UART_INTR_DISABLE(); // NOLINT hicpp-signed-bitwise
}
InterruptLock::~InterruptLock() noexcept {
  ETS_UART_INTR_ENABLE(); // NOLINT hicpp-signed-bitwise
  IOP_TRACE();
}
}