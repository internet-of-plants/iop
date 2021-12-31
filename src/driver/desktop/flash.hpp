#include "driver/flash.hpp"
#include "driver/panic.hpp"
#include <fstream>

namespace driver {
// This driver is horrible, please fix this
// Use fopen, properly report errors, keep the file open, memmap (?)...

void Flash::setup(size_t size) noexcept {
    if (size == 0) return;
    this->buffer = new (std::nothrow) uint8_t[size];
    iop_assert(this->buffer, IOP_STATIC_STRING("Allocation failed"));
    std::memset(this->buffer, '\0', size);

    this->size = size;

    std::ifstream file("eeprom.dat");
    if (!file.is_open()) {
        return;
    }

    file.read((char*) buffer, size);
    iop_assert(!file.fail(), IOP_STATIC_STRING("Read failed"));

    file.close();
    iop_assert(!file.fail(), IOP_STATIC_STRING("Close failed"));
}
void Flash::commit() noexcept {
    iop_assert(this->buffer, IOP_STATIC_STRING("Unable to allocate storage"));
    //iop::Log(logLevel, IOP_STATIC_STRING("EEPROM")).debug(IOP_STATIC_STRING("Commit: "), utils::base64Encode(this->storage.get(), this->size));
    std::ofstream file("eeprom.dat");
    iop_assert(file.is_open(), IOP_STATIC_STRING("Unable to open eeprom.dat file"));

    file.write((char*) buffer, size);
    iop_assert(!file.fail(), IOP_STATIC_STRING("Write failed"));

    file.close();
    iop_assert(!file.fail(), IOP_STATIC_STRING("Close failed"));
}
uint8_t const * Flash::asRef() const noexcept {
    iop_assert(this->buffer, IOP_STATIC_STRING("Allocation failed"));
    return this->buffer;
}
uint8_t * Flash::asMut() noexcept {
    iop_assert(this->buffer, IOP_STATIC_STRING("Allocation failed"));
    return this->buffer;
}
}