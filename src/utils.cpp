#include "utils.hpp"
#include "models.hpp"

#ifdef IOP_DESKTOP
#define IRAM_ATTR
#endif

static volatile InterruptEvent interruptEvents[interruptVariants] = {
    InterruptEvent::NONE};

namespace utils {
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
// This function is called inside an interrupt, it can't be fancy (it can only
// call functions stored in IRAM_ATTR)
void IRAM_ATTR scheduleInterrupt(const InterruptEvent ev) noexcept {
  volatile InterruptEvent *ptr = nullptr;
  for (volatile auto &el : interruptEvents) {
    if (el == InterruptEvent::NONE && ptr == nullptr) {
      ptr = &el;

      // We only allow one of each interrupt concurrently
      // So if we find ourselves we bail
      if (el == ev)
        return;
    }

    if (ptr != nullptr) {
      *ptr = ev;
    } else {
      // If no space is available and we reach here there is a bug in the code,
      // interruptVariants is probably wrong. We used to panic here, but that
      // doesn't work when called from interrupts
      iop::Log::print(F("[CRIT] RESET: Unsable to store interrupt, "
                        "'interruptVariant' is probably wrong\n"),
                      iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}
} // namespace utils
