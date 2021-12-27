#ifdef IOP_DESKTOP
#include "driver/desktop/flash.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/flash.hpp"
#elif defined(IOP_NOOP)
#include "driver/noop/flash.hpp"
#else
#error "Target not supported"
#endif

namespace driver {
Flash flash;

std::optional<uint8_t> Flash::read(const size_t address) const noexcept {
    if (address >= this->size) return std::optional<uint8_t>();
    return this->asRef()[address];
}

void Flash::write(const size_t address, uint8_t const val) noexcept {
    iop_assert(this->buffer, FLASH("Buffer is nullptr"));
    iop_assert(address < this->size, FLASH("Write to invalid address"));
    this->asMut()[address] = val;
}
}