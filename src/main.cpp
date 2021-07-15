#include "loop.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

Unused4KbSysStack unused4KbSysStack;
void setup() {
  panic::setup();
  network_logger::setup();
  unused4KbSysStack.loop().setup();
}
void loop() { unused4KbSysStack.loop().loop(); }