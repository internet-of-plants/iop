#include "driver/flash.hpp"
#include "driver/panic.hpp"

namespace driver {
void Flash::setup(size_t size) noexcept {
    this->size = size;
    if (!buffer) {
        buffer = new (std::nothrow) uint8_t[size];
        memset(buffer, 0, size);
    }
}
void Flash::commit() noexcept {}
uint8_t const * Flash::asRef() const noexcept { if (!buffer) iop_panic(IOP_STATIC_STRING("Buffer is nullptr")); return buffer; }
uint8_t * Flash::asMut() noexcept { if (!buffer) iop_panic(IOP_STATIC_STRING("Buffer is nullptr")); return buffer; }
}