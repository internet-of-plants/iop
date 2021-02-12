#ifndef IOP_CORE_UTILS_HPP
#define IOP_CORE_UTILS_HPP

#include "storage.hpp"

// (Un)Comment this line to toggle wifi dependency
#define IOP_ONLINE

// (Un)Comment this line to toggle serial dependency
#define IOP_SERIAL

// (Un)Comment this line to toggle SSL dependency
// #define IOP_SSL

// (Un)Comment this line to toggle memory stats logging
// #define LOG_MEMORY

namespace iop {
using esp_time = unsigned long; // NOLINT google-runtime-int

TYPED_STORAGE(MD5Hash, 32);
TYPED_STORAGE(MacAddress, 17);

auto hashSketch() noexcept -> const MD5Hash &;
auto macAddress() noexcept -> const MacAddress &;
void logMemory(const iop::Log &logger) noexcept;
} // namespace iop

#endif