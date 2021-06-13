#include "driver/gpio.hpp"

namespace gpio {
    gpio::GPIO gpio;
}

#ifdef IOP_DESKTOP
namespace gpio {
void GPIO::mode(const Pin pin, const Mode mode) const noexcept {
    (void) pin;
    (void) mode;
}
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data {
    (void) pin;
    return Data::HIGH;
}
void GPIO::alarm(const Pin pin, const Alarm mode, void (*func)()) const noexcept {
    (void) pin;
    (void) mode;
    (void) func;
}
}
#else

#include "Arduino.h"
#undef HIGH
#undef LOW

namespace gpio {
void GPIO::mode(const Pin pin, const Mode mode) const noexcept {
    ::pinMode(static_cast<uint8_t>(pin), static_cast<uint8_t>(mode));
}
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data {
    return ::digitalRead(static_cast<uint8_t>(pin)) ? Data::HIGH : Data::LOW;
}
void GPIO::alarm(const Pin pin, const Alarm mode, void (*func)()) const noexcept {
    ::attachInterrupt(digitalPinToInterrupt(static_cast<uint8_t>(pin)), func, static_cast<uint8_t>(mode));
}
}
#endif