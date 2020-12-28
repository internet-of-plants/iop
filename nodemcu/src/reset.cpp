#include <reset.hpp>

#ifndef IOP_FACTORY_RESET_DISABLED
#include <Arduino.h>
#include <configuration.h>
#include <log.hpp>
#include <utils.hpp>

PROGMEM_STRING(logTarget, "INTERRUPT");
const Log logger(logLevel, logTarget, false);

volatile unsigned long resetStateTime = 0;
void ICACHE_RAM_ATTR buttonClosed() noexcept;

void ICACHE_RAM_ATTR buttonOpen() noexcept {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed,
                  RISING);
  if (resetStateTime + 15000 < millis()) {
    interruptEvent = FACTORY_RESET;
    logger.info(
        F("Setted FACTORY_RESET flag, running it in the next loop run"));
  }
}

void ICACHE_RAM_ATTR buttonClosed() noexcept {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonOpen,
                  FALLING);
  resetStateTime = millis();
  logger.info(F("Pressed FACTORY_RESET button. Keep it pressed for at least 15 "
                "seconds to factory reset your device"));
}

namespace reset {
void setup() noexcept {
  pinMode(factoryResetButton, INPUT);
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed,
                  RISING);
}
} // namespace reset
#endif

#ifdef IOP_FACTORY_RESET_DISABLED
namespace reset {
void setup() {}
} // namespace reset
#endif
