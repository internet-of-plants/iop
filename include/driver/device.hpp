#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include <stddef.h>
#include <stdint.h>
#include <array>
#include "core/string.hpp"

class String;

#ifdef IOP_DESKTOP
class HeapSelectIram {
  uint8_t dummy = 0;
};
class HeapSelectDram {
  uint8_t dummy = 0;
};
#else
#include <umm_malloc/umm_heap_select.h>
#endif

namespace driver {
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