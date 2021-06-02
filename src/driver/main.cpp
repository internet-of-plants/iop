#ifdef IOP_DESKTOP
#ifndef UNIT_TEST
#include <unistd.h>
#include <iostream>
void setup();
void loop();

int main(int argc, char** argv) {
  setup();
  while (true) {
    loop();
    usleep(100);
  }
  return 0;
}
#endif
#endif