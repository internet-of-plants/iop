#ifndef IOP_DRIVER_FLASH_HPP
#define IOP_DRIVER_FLASH_HPP

#include <stdint.h>
#include <stddef.h>
#include <optional>

namespace driver {
class Flash {
  size_t size = 0;
  bool shouldCommit = false;
  uint8_t *buffer = nullptr;
public:
  void setup(size_t size) noexcept;
  std::optional<uint8_t> read(size_t address) const noexcept;
  void write(size_t address, uint8_t val) noexcept;
  void commit() noexcept;
  uint8_t const * asRef() const noexcept;
  uint8_t * asMut() noexcept;

  template<typename T> 
  void put(int const address, const T &t) {
    if (address + sizeof(T) > size) return;
    memcpy(this->buffer + address, &t, sizeof(T));
  }
};
extern Flash flash;
}

#endif