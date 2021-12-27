#include "loop.hpp"

#if defined(IOP_ESP8266) || ARDUINO
#include "Arduino.h"
#elif !defined(UNIT_TEST)
#include "driver/thread.hpp"

void setup();
void loop();

int main(int argc, char** argv) {
  (void) argc;
  (void) argv;
  
  setup();
  while (true) {
    loop();
    driver::thisThread.sleep(100);
  }
  return 0;
}
#endif
// TODO: log restart reason Esp::getResetInfoPtr()

Unused4KbSysStack unused4KbSysStack;
void setup() {
  panic::setup();
  network_logger::setup();
  eventLoop.setup();
}
void loop() { eventLoop.loop(); }