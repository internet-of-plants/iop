#include "loop.hpp"
#ifndef IOP_DESKTOP
#include "Arduino.h"
#endif
// TODO: log restart reason Esp::getResetInfoPtr()

Unused4KbSysStack unused4KbSysStack;
void setup() {
  panic::setup();
  network_logger::setup();
  eventLoop.setup();
}
void loop() { eventLoop.loop(); }