#ifndef IOP_DRIVER_FLASH_HPP
#define IOP_DRIVER_FLASH_HPP

#ifdef IOP_DESKTOP
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <errno.h>
#include "configuration.hpp"
#include "core/panic.hpp"

#include "utils.hpp"

static int read__(int fd, void* buf, size_t size) {
  return read(fd, buf, size);
}
static int write_(int fd, const void* buf, size_t size) {
  return write(fd, buf, size);
}

class Eeprom {
  size_t size;
  std::unique_ptr<uint8_t[]> storage;
public:
  Eeprom(): size(0), storage(nullptr) {}

  void begin(size_t size) {
    if (size == 0) return;
    this->size = size;
    this->storage = iop::try_make_unique<uint8_t[]>(size);
    if (!this->storage) return;
    memset(this->storage.get(), '\0', size);
    
    const auto fd = open("eeprom.dat", O_RDONLY);
    if (fd == -1) return;
    if (read__(fd, this->storage.get(), size) == -1) return;
    close(fd);
  }
  uint8_t read(int const address) {
    if (address < 0 || address >= this->size) return 0;
    if (!this->storage) return 0;
    return this->storage[address];
  }
  void write(int const address, uint8_t const val) {
    iop_assert(address >= 0 || address < this->size, F("Invalid address"));
    iop_assert(this->storage, F("Allocation failed"));
    this->storage[address] = val;
  }
  bool commit() {
    iop_assert(this->storage, F("Unable to allocate storage"));
    //iop::Log(logLevel, F("EEPROM")).debug(F("Commit: "), utils::base64Encode(this->storage.get(), this->size));
    const auto fd = open("eeprom.dat", O_WRONLY | O_CREAT, 0777);
    iop_assert(fd != -1, F("Unable to open file"));
    if (write_(fd, this->storage.get(), size) == -1) {
      iop_panic(std::to_string(errno) + ": " + strerror(errno));
    }
    
    iop_assert(close(fd) != -1, F("Close failed"));
    return true;
  }
  bool end() {
    const auto ret = this->commit();
    this->storage.reset(nullptr);
    return ret;
  }

  uint8_t * getDataPtr() {
    iop_assert(this->storage, F("Allocation failed"));
    return this->storage.get();
  }
  uint8_t const * getConstDataPtr() const {
    iop_assert(this->storage, F("Allocation failed"));
    return this->storage.get();
  }

  template<typename T> 
  T &get(int const address, T &t) {
    iop_assert(address < 0 || address + sizeof(T) > size, F("Invalid address"));
    memcpy((uint8_t*) &t, this->storage.get() + address, sizeof(T));
    return t;
  }

  template<typename T> 
  const T &put(int const address, const T &t) {
    if (address < 0 || address + sizeof(T) > size) return t;
    memcpy(this->storage.get() + address, (const uint8_t*)&t, sizeof(T));
    return t;
  }

  size_t length() { return size; }
  uint8_t& operator[](int const address) { return getDataPtr()[address]; }
  uint8_t const & operator[](int const address) const { return getConstDataPtr()[address]; }
};

static Eeprom EEPROM;
#else
#include "EEPROM.h"
#endif

#endif