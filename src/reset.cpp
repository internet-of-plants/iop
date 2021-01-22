#include "reset.hpp"

#ifndef IOP_FACTORY_RESET_DISABLED
#include "Arduino.h"
#include "configuration.h"
#include "log.hpp"
#include "models.hpp"
#include "utils.hpp"


PROGMEM_STRING(logTarget, "INTERRUPT");
const static Log logger(logLevel, logTarget, false);

static volatile esp_time resetStateTime = 0;

void ICACHE_RAM_ATTR buttonChanged() noexcept {
  if (digitalRead(factoryResetButton) == HIGH) {
    resetStateTime = millis();
    logger.info(
        F("Pressed FACTORY_RESET button. Keep it pressed for at least 15 "
          "seconds to factory reset your device"));
  } else {
    constexpr const uint32_t fifteenSeconds = 15000;
    if (resetStateTime + fifteenSeconds < millis()) {
      utils::scheduleInterrupt(InterruptEvent::FACTORY_RESET);
      logger.info(
          F("Setted FACTORY_RESET flag, running it in the next loop run"));
    }
  }
}

namespace reset {
void setup() noexcept {
  pinMode(factoryResetButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonChanged,
                  CHANGE);
}
} // namespace reset
#else
namespace reset {
void setup() {}
} // namespace reset
#endif
