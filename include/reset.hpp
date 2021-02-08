#ifndef IOP_RESET_HPP
#define IOP_RESET_HPP

/// Abstracts factory resets
namespace reset {
void setup() noexcept;
} // namespace reset

#include "utils.hpp"
#ifndef IOP_FACTORY_RESET
#define IOP_FACTORY_RESET_DISABLED
#endif

#endif