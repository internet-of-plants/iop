#include "driver/flash.hpp"

namespace driver {
    Flash flash;
}

#ifdef IOP_DESKTOP
#include <unistd.h>
#include <errno.h>
#include "core/panic.hpp"
#include <new>
#include <cstdlib>
#include <fcntl.h>

namespace driver {
// This driver is horrible, please fix this
// Use fopen, properly report errors, keep the file open, memmap (?)...

void Flash::setup(size_t size) noexcept {
    IOP_TRACE();
    if (size == 0) return;
    this->buffer = new (std::nothrow) uint8_t[size];
    iop_assert(this->buffer, F("Allocation failed"));
    std::memset(this->buffer, '\0', size);

    this->size = size;

    const auto fd = ::open("eeprom.dat", O_RDONLY);
    if (fd != -1) {
        if (::read(fd, this->buffer, size) == -1) {
            ::free(this->buffer);
            this->buffer = nullptr;
            this->size = 0;
            return;
        }
        close(fd);
    }
}
std::optional<uint8_t> Flash::read(const size_t address) const noexcept {
    IOP_TRACE();
    if (address >= this->size) return std::optional<uint8_t>();
    return this->buffer[address];
}
void Flash::write(const size_t address, uint8_t const val) noexcept {
    IOP_TRACE();
    iop_assert(this->buffer, F("Unable to allocate buffer"));
    if (address >= this->size) return;
    this->shouldCommit = true;
    this->buffer[address] = val;
}
void Flash::commit() noexcept {
    IOP_TRACE();
    iop_assert(this->buffer, F("Unable to allocate storage"));
    //iop::Log(logLevel, F("EEPROM")).debug(F("Commit: "), utils::base64Encode(this->storage.get(), this->size));
    const auto fd = ::open("eeprom.dat", O_WRONLY | O_CREAT, 0777);
    iop_assert(fd != -1, F("Unable to open file"));
    if (::write(fd, this->buffer, size) == -1) {
      iop_panic(std::to_string(errno) + ": " + strerror(errno));
    }
    
    iop_assert(::close(fd) != -1, F("Close failed"));
}
uint8_t const * Flash::asRef() const noexcept {
    IOP_TRACE();
    iop_assert(this->buffer, F("Allocation failed"));
    return this->buffer;
}
uint8_t * Flash::asMut() noexcept {
    IOP_TRACE();
    iop_assert(this->buffer, F("Allocation failed"));
    return this->buffer;
}
}
#else
#include "EEPROM.h"

static EEPROMClass EEPROM;

namespace driver {
// TODO: properly handle EEPROM internal errors
void Flash::setup(size_t size) noexcept {
    EEPROM.begin(size);
}
std::optional<uint8_t> Flash::read(const size_t address) const noexcept {
    return EEPROM.read(address);
}
void Flash::write(const size_t address, uint8_t const val) noexcept {
    EEPROM.write(address, val);
}
void Flash::commit() noexcept {
    EEPROM.commit();
}
uint8_t const * Flash::asRef() const noexcept {
    return EEPROM.getConstDataPtr();
}
uint8_t * Flash::asMut() noexcept {
    return EEPROM.getDataPtr();
}
}
#endif