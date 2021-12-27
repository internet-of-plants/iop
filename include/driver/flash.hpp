#ifndef IOP_DRIVER_FLASH_HPP
#define IOP_DRIVER_FLASH_HPP

#include "driver/panic.hpp"
#include <stdint.h>
#include <stddef.h>
#include <optional>

namespace driver {
class Flash {
  size_t size = 0;
  uint8_t *buffer = nullptr;
  
public:
  void setup(size_t size) noexcept;
  auto read(size_t address) const noexcept -> std::optional<uint8_t>;
  void write(size_t address, uint8_t val) noexcept;
  void commit() noexcept;
  auto asRef() const noexcept -> uint8_t const *;
  auto asMut() noexcept -> uint8_t *;

  template<typename T> 
  void put(int const address, const T &t) {
    iop_assert(address + sizeof(T) <= this->size, FLASH("Flash overflow: ").toString() + std::to_string(address + sizeof(T)));
    memcpy(this->asMut() + address, &t, sizeof(T));
  }
};
extern Flash flash;
}

#endif