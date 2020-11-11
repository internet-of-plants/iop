#include <reset.hpp>

#ifndef IOP_FACTORY_RESET_DISABLED
#include <Arduino.h>
#include <configuration.h>
#include <log.hpp>
#include <utils.hpp>

const char * const logTarget PROGMEM = "INTERRUPT";
const Log logger(logLevel, StaticString(logTarget), false);

volatile unsigned long resetStateTime = 0;
void ICACHE_RAM_ATTR buttonClosed();

void ICACHE_RAM_ATTR buttonOpen() {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed, RISING);
  if (resetStateTime + 15000 < millis()) {
    interruptEvent = FACTORY_RESET;
    logger.info(STATIC_STRING("Setted FACTORY_RESET flag, running it in the next loop run"));
  }
}

void ICACHE_RAM_ATTR buttonClosed() {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonOpen, FALLING);
  resetStateTime = millis();
  logger.info(STATIC_STRING("Pressed FACTORY_RESET button. Keep it pressed for at least 15 seconds to factory reset your device"));
}

namespace reset {
  void setup() {
    pinMode(factoryResetButton, INPUT);
    attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed, RISING);
  }
}
#endif

#ifdef IOP_FACTORY_RESET_DISABLED
namespace reset { void setup() {} }
#endif
