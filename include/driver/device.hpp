#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include <stddef.h>
#include <stdint.h>
#include <array>

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
};
extern Device device;
}

#endif