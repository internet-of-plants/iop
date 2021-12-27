#include "driver/flash.hpp"
#include "driver/panic.hpp"
#include "EEPROM.h"

// TODO: move this to global structure
//EEPROMClass EEPROM;

namespace driver {
// TODO: properly handle EEPROM internal errors
void Flash::setup(size_t size) noexcept {
    this->size = size;
    EEPROM.begin(size);
}
void Flash::commit() noexcept {
    // TODO: report errors in flash usage
    iop_assert(EEPROM.commit(), FLASH("EEPROM commit failed"));
}
uint8_t const * Flash::asRef() const noexcept {
    return EEPROM.getConstDataPtr();
}
uint8_t * Flash::asMut() noexcept {
    return EEPROM.getDataPtr();
}
}