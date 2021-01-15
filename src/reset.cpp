#include "reset.hpp"

#ifndef IOP_FACTORY_RESET_DISABLED
#include "Arduino.h"
#include "configuration.h"
#include "log.hpp"
#include "utils.hpp"

PROGMEM_STRING(logTarget, "INTERRUPT");
const Log logger(logLevel, logTarget, false);

volatile unsigned long resetStateTime = 0; // NOLINT google-runtime-int
void ICACHE_RAM_ATTR buttonChanged() noexcept;

void ICACHE_RAM_ATTR buttonChanged() noexcept {
  if (digitalRead(factoryResetButton) == HIGH) {
    resetStateTime = millis();
    logger.info(
        F("Pressed FACTORY_RESET button. Keep it pressed for at least 15 "
          "seconds to factory reset your device"));
  } else {
    constexpr const uint32_t fifteenSeconds = 15000;
    if (resetStateTime + fifteenSeconds < millis()) {
      interruptEvent = FACTORY_RESET;
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
#endif

#ifdef IOP_FACTORY_RESET_DISABLED
namespace reset {
void setup() {}
} // namespace reset
#endif
