#ifndef IOP_DRIVER_DEVICE_HPP
#define IOP_DRIVER_DEVICE_HPP

#include "core/string.hpp"
#include <stddef.h>
#include <stdint.h>
#include <array>

#ifdef IOP_DESKTOP
#define IRAM_ATTR
#else
#define __ICACHE_STRINGIZE_NX(A) #A
#define __ICACHE_STRINGIZE(A) __ICACHE_STRINGIZE_NX(A)
#define IRAM_ATTR __attribute__((section("\".iram.text." __FILE__ "." __ICACHE_STRINGIZE(__LINE__) "." __ICACHE_STRINGIZE(__COUNTER__) "\"")))
#endif

class String;

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
  void deepSleep(uint32_t seconds) const noexcept;
  std::array<char, 32>& binaryMD5() const noexcept;
  std::array<char, 17>& macAddress() const noexcept;
  ::iop::StaticString platform() const noexcept;
};
extern Device device;
}

#endif