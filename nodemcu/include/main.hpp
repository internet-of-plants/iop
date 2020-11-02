#ifndef IOP_MAIN_H_
#define IOP_MAIN_H_

const uint8_t oneWireBus = D6;
const uint8_t dhtPin = D7;

const unsigned long timerDelay = 2 * 1000;

int main(int argc, char ** argv);

#endif