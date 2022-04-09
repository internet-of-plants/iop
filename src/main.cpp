#include "iop/loop.hpp"

// TODO: log restart reason Esp::getResetInfoPtr()

namespace iop_hal {
auto setup() noexcept -> void { iop::eventLoop.setup(); }
auto loop() noexcept -> void { iop::eventLoop.loop(); }
}