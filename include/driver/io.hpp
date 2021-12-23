#ifndef IOP_DRIVER_PINS_HPP
#define IOP_DRIVER_PINS_HPP

namespace driver {
namespace io {
enum class Mode {
  INPUT = 0,
  OUTPUT = 1,
};
enum class Data {
  LOW = 0,
  HIGH = 1,
};
enum class Pin {
  D1 = 5,
  D2 = 4,
  D5 = 14,
  D6 = 12,
  D7 = 13
};
constexpr static  Pin LED_BUILTIN = Pin::D2;

enum class Alarm {
  RISING = 1,
  FALLING = 2,
  CHANGE = 3,
};
class GPIO {
public:
  void mode(Pin pin, Mode mode) const noexcept;
  auto digitalRead(Pin pin) const noexcept -> Data;
  void alarm(Pin pin, Alarm mode, void (*func)()) const noexcept;
};
} // namespace io

extern io::GPIO gpio;
} // namespace driver

#endif