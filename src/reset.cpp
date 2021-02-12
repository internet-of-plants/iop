#include "reset.hpp"
#include "utils.hpp"

#ifdef IOP_FACTORY_RESET
#include "configuration.hpp"

static volatile iop::esp_time resetStateTime = 0;

void ICACHE_RAM_ATTR buttonChanged() noexcept {
  // IOP_TRACE();
  if (digitalRead(factoryResetButton) == HIGH) {
    resetStateTime = millis();
    if (logLevel >= iop::LogLevel::INFO)
      iop::Log::print(F("[INFO] RESET: Pressed FACTORY_RESET button. Keep it "
                        "pressed for at least 15 "
                        "seconds to factory reset your device\n"),
                      iop::LogLevel::INFO, iop::LogType::STARTEND);
  } else {
    constexpr const uint32_t fifteenSeconds = 15000;
    if (resetStateTime + fifteenSeconds < millis()) {
      utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
      if (logLevel >= iop::LogLevel::INFO)
        iop::Log::print(
            F("[INFO] RESET: Setted FACTORY_RESET flag, running it in "
              "the next loop run\n"),
            iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}

namespace reset {
void setup() noexcept {
  IOP_TRACE();
  pinMode(factoryResetButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonChanged,
                  CHANGE);
}
} // namespace reset
#else
namespace reset {
void setup() noexcept { IOP_TRACE(); }
} // namespace reset
#endif
