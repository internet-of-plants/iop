#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "core/string.hpp"
#include <stddef.h>
#include <stdint.h>
#include <array>

class String;

#ifndef IOP_DESKTOP
class HeapSelectIram;
class HeapSelectDram;
#endif

namespace driver {
#ifdef IOP_DESKTOP
class HeapSelectIram {
  uint8_t dummy = 0;
};
class HeapSelectDram {
  uint8_t dummy = 0;
};
#else
class HeapSelectIram {
  ::HeapSelectIram *ptr;
public:
  HeapSelectIram() noexcept;
  ~HeapSelectIram() noexcept;
};
class HeapSelectDram {
  ::HeapSelectDram *ptr;
public:
  HeapSelectDram() noexcept;
  ~HeapSelectDram() noexcept;
};
#endif

class Device {
public:
  auto availableFlash() const noexcept -> size_t;
  auto availableStack() const noexcept -> size_t;
  auto availableHeap() const noexcept -> size_t;
  auto vcc() const noexcept -> uint16_t;
  auto biggestHeapBlock() const noexcept -> size_t;
  void deepSleep(uint32_t seconds) const noexcept;
  std::array<char, 32>& binaryMD5() const noexcept;
  std::array<char, 17>& macAddress() const noexcept;
  ::iop::StaticString platform() const noexcept;
};
extern Device device;
}

#endif