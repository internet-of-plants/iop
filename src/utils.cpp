#include "core/log.hpp"
#include "driver/device.hpp"
#include "utils.hpp"

static volatile InterruptEvent interruptEvents[interruptVariants] = { InterruptEvent::NONE, InterruptEvent::NONE, InterruptEvent::NONE, InterruptEvent::NONE };

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
    iop::Log::print(FLASH("[CRIT] RESET: Unable to store interrupt, "
                      "'interruptVariant' is probably wrong\n"),
                    iop::LogLevel::INFO, iop::LogType::STARTEND);
  }
}

auto base64Encode(const uint8_t *in, const size_t size) noexcept -> std::string {
  std::string out;

  int val = 0, valb = -6;
  for (uint32_t index = 0; index < size; ++index) {
    const char c = in[index];
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val>>valb)&0x3F]);
      valb -= 6;
    }
  }
  if (valb>-6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val<<8)>>(valb+8))&0x3F]);
  while (out.size()%4) out.push_back('=');
  return out;
}
} // namespace utils
