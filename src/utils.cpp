#include "iop-hal/log.hpp"
#include "iop-hal/device.hpp"
#include "iop/utils.hpp"

namespace iop {
static volatile InterruptEvent interruptEvents[interruptVariants] = { InterruptEvent::NONE, InterruptEvent::NONE, InterruptEvent::NONE };

auto descheduleInterrupt() noexcept -> InterruptEvent {
  IOP_TRACE();
  for (volatile auto &el : interruptEvents) {
    if (el != InterruptEvent::NONE) {
      const auto tmp = el;
      el = InterruptEvent::NONE;
      return tmp;
    }
  }
  return InterruptEvent::NONE;
}
// This function is called inside an interrupt, it can't be fancy (it can only call functions stored in IOP_RAM)
void IOP_RAM scheduleInterrupt(const InterruptEvent ev) noexcept {
  volatile InterruptEvent *ptr = nullptr;
  for (volatile auto &el : interruptEvents) {
    if (el == InterruptEvent::NONE && ptr == nullptr) {
      ptr = &el;
    } else if (el == ev) {
      return;
    } else if (el != InterruptEvent::NONE) {
      continue;
    }
  }

  if (ptr != nullptr) {
    *ptr = ev;
  } else {
    // If no space is available and we reach here there is a bug in the code,
    // interruptVariants is probably wrong. We used to panic here, but that
    // doesn't work when called from interrupts
    iop::Log::print("[CRIT] RESET: Unable to store interrupt, 'interruptVariant' is probably wrong\n",
                    iop::LogLevel::INFO, iop::LogType::STARTEND);
  }
}
}
