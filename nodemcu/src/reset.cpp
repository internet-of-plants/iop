#include <reset.hpp>
#include <configuration.h>
#include <log.hpp>
#include <utils.hpp>

const Log logger(logLevel, "RESET");

volatile unsigned long resetStateTime = 0; 
void buttonClosed();

void ICACHE_RAM_ATTR buttonOpen() {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed, RISING);
  if (resetStateTime + 15000 < millis()) {
    interruptEvent = FACTORY_RESET;
    logger.info("Setted FACTORY_RESET flag, running it in the next loop run");
  }
}

void ICACHE_RAM_ATTR buttonClosed() {
  detachInterrupt(digitalPinToInterrupt(factoryResetButton));
  attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonOpen, FALLING);
  resetStateTime = millis();
  logger.info("Pressed FACTORY_RESET button. Keep it pressed for at least 15 seconds to factory reset your device");
}

namespace reset {
  void setup() {
    pinMode(factoryResetButton, INPUT);
    attachInterrupt(digitalPinToInterrupt(factoryResetButton), buttonClosed, RISING);
  }
}
