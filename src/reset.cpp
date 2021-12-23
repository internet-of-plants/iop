#ifdef IOP_FACTORY_RESET
#include "driver/device.hpp"
#include "driver/io.hpp"
#include "driver/thread.hpp"
#include "configuration.hpp"

static volatile iop::esp_time resetStateTime = 0;

void IRAM_ATTR buttonChanged() noexcept {
  // IOP_TRACE();
  if (driver::gpio.digitalRead(config::factoryResetButton) == driver::io::Data::HIGH) {
    resetStateTime = driver::thisThread.now();
    if (config::logLevel >= iop::LogLevel::INFO)
      iop::Log::print(FLASH("[INFO] RESET: Pressed FACTORY_RESET button. Keep it "
                        "pressed for at least 15 "
                        "seconds to factory reset your device\n"),
                      iop::LogLevel::INFO, iop::LogType::STARTEND);
  } else {
    constexpr const uint32_t fifteenSeconds = 15000;
    if (resetStateTime + fifteenSeconds < driver::thisThread.now()) {
      utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
      if (config::logLevel >= iop::LogLevel::INFO)
        iop::Log::print(
            FLASH("[INFO] RESET: Setted FACTORY_RESET flag, running it in "
              "the next loop run\n"),
            iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}

namespace reset {
void setup() noexcept {
  IOP_TRACE();
  driver::gpio.mode(config::factoryResetButton, driver::io::Mode::INPUT);
  driver::gpio.alarm(config::factoryResetButton, driver::io::Alarm::CHANGE, buttonChanged);
}
} // namespace reset
#else
namespace reset {
void setup() noexcept { IOP_TRACE(); }
} // namespace reset
#endif
