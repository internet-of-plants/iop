#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include <stddef.h>
#include <stdint.h>

namespace iop {
class MD5Hash_class;
class MacAddress_class;
}

namespace driver {
class Device {
public:
  auto availableFlash() const noexcept -> size_t;
  auto availableStack() const noexcept -> size_t;
  auto availableHeap() const noexcept -> size_t;
  auto biggestHeapBlock() const noexcept -> size_t;
  void deepSleep(uint32_t seconds) const noexcept;
  iop::MD5Hash_class & binaryMD5() const noexcept;
  iop::MacAddress_class & macAddress() const noexcept;
};
extern Device device;
}

#endif