#ifndef IOP_RESET_H_
#define IOP_RESET_H_

namespace reset {
void setup() noexcept;
}

#include <utils.hpp>
#ifndef IOP_FACTORY_RESET
#define IOP_FACTORY_RESET_DISABLED
#endif

#endif