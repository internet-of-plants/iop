#include "driver/flash.hpp"
#include "driver/panic.hpp"
#include <fstream>

namespace driver {
// This driver is horrible, please fix this
// Use fopen, properly report errors, keep the file open, memmap (?)...

void Flash::setup(size_t size) noexcept {
    if (size == 0) return;
    this->buffer = new (std::nothrow) uint8_t[size];
    iop_assert(this->buffer, FLASH("Allocation failed"));
    std::memset(this->buffer, '\0', size);

    this->size = size;

    std::ifstream file("eeprom.dat");
    if (!file.is_open()) {
        return;
    }

    file.read((char*) buffer, size);
    iop_assert(!file.fail(), FLASH("Read failed"));

    file.close();
    iop_assert(!file.fail(), FLASH("Close failed"));
}
void Flash::commit() noexcept {
    iop_assert(this->buffer, FLASH("Unable to allocate storage"));
    //iop::Log(logLevel, FLASH("EEPROM")).debug(FLASH("Commit: "), utils::base64Encode(this->storage.get(), this->size));
    std::ofstream file("eeprom.dat");
    iop_assert(file.is_open(), FLASH("Unable to open eeprom.dat file"));

    file.write((char*) buffer, size);
    iop_assert(!file.fail(), FLASH("Write failed"));

    file.close();
    iop_assert(!file.fail(), FLASH("Close failed"));
}
uint8_t const * Flash::asRef() const noexcept {
    iop_assert(this->buffer, FLASH("Allocation failed"));
    return this->buffer;
}
uint8_t * Flash::asMut() noexcept {
    iop_assert(this->buffer, FLASH("Allocation failed"));
    return this->buffer;
}
}