#ifndef IOP_CORE_UTILS_HPP
#define IOP_CORE_UTILS_HPP

#include <array>
#include "core/log.hpp"

namespace iop {
using MD5Hash = std::array<char, 32>;
using MacAddress = std::array<char, 17>;

void logMemory(const Log &logger) noexcept;

} // namespace iop

#endif