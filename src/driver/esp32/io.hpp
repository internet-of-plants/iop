#include "driver/io.hpp"

namespace driver {
namespace io {
void GPIO::mode(const Pin pin, const Mode mode) const noexcept { (void) pin; (void) mode; }
auto GPIO::digitalRead(const Pin pin) const noexcept -> Data { (void) pin; return Data::HIGH; }
void GPIO::alarm(const Pin pin, const Alarm mode, void (*func)()) const noexcept { (void) pin; (void) mode; (void) func; }
} // namespace io
} // namespace driver