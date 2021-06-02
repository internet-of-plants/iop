#ifndef IOP_DRIVER_PINS_HPP
#define IOP_DRIVER_PINS_HPP

#ifdef IOP_DESKTOP
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;

static const bool HIGH = true;
static const bool LOW = true;

static const uint8_t INPUT = 0;
static const uint8_t OUTPUT = 1;

static const bool LED_BUILTIN = 0;
#define RISING    0x01
#define FALLING   0x02
#define CHANGE    0x03
#define ONLOW     0x04
#define ONHIGH    0x05
#define ONLOW_WE  0x0C
#define ONHIGH_WE 0x0D
static void pinMode(uint8_t pin, uint8_t mode) {
  (void) pin;
  (void) mode;
}

#define EXTERNAL_NUM_INTERRUPTS 16
#define NOT_AN_INTERRUPT -1
#define digitalPinToInterrupt(p)    (((p) < EXTERNAL_NUM_INTERRUPTS)? (p) : NOT_AN_INTERRUPT)

static void attachInterrupt(uint8_t pin, std::function<void(void)> intRoutine, int mode) {
    (void)pin;
    (void)intRoutine;
    (void)mode;    
}

static auto digitalRead(const uint8_t pin) -> bool {
    (void)pin;
    return LOW;
}
#else
#include "pins_arduino.h"
#include "Arduino.h"
#endif

#endif