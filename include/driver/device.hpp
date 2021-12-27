#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "driver/string.hpp"
#include <stdint.h>
#include <array>

#if defined(IOP_ESP8266) || (defined(ARDUINO) && defined(IOP_NOOP))
#include <c_types.h>
#else
#define IRAM_ATTR
#endif

class HeapSelectIram;
class HeapSelectDram;

namespace driver {
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

class Device {
public:
  void syncNTP() const noexcept;
  auto availableFlash() const noexcept -> size_t;
  auto availableStack() const noexcept -> size_t;
  auto availableHeap() const noexcept -> size_t;
  auto vcc() const noexcept -> uint16_t;
  auto biggestHeapBlock() const noexcept -> size_t;
  void deepSleep(size_t seconds) const noexcept;
  std::array<char, 32>& binaryMD5() const noexcept;
  std::array<char, 17>& macAddress() const noexcept;
  ::iop::StaticString platform() const noexcept;
};
extern Device device;
}

#endif