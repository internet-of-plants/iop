#ifndef IOP_RESET_H
#define IOP_RESET_H

namespace reset {
void setup() noexcept;
}

#include "utils.hpp"
#ifndef IOP_FACTORY_RESET
#define IOP_FACTORY_RESET_DISABLED
#endif

#endif