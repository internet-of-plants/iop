#include "driver/io.hpp"
#include "Arduino.h"
#undef HIGH
#undef LOW

namespace driver {
namespace io {
void GPIO::mode(const Pin pin, const Mode mode) const noexcept {
    ::pinMode(static_cast<uint8_t>(pin), static_cast<uint8_t>(mode));
}
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data {
    return ::digitalRead(static_cast<uint8_t>(pin)) ? Data::HIGH : Data::LOW;
}
void GPIO::alarm(const Pin pin, const Alarm mode, void (*func)()) const noexcept {
    ::attachInterrupt(digitalPinToInterrupt(static_cast<uint8_t>(pin)), func, static_cast<uint8_t>(mode));
}
} // namespace io
} // namespace driver