#ifndef IOP_PANIC_HPP
#define IOP_PANIC_HPP

#include "core/string/view.hpp"

namespace iop {
void panic_hook(StringView msg, const StaticString &file, uint32_t line,
                const StringView &func) noexcept __attribute__((noreturn));

void panic_hook(StaticString msg, const StaticString &file, uint32_t line,
                const StringView &func) noexcept __attribute__((noreturn));
} // namespace iop

/// Panic. Never returns. Logs panic to network if available (and serial).
/// If any update is available it's installed and reboots.
///
/// Sent: Message + Line + Function + File
#define iop_panic(msg) iop::panic_hook((msg), IOP_FILE, IOP_LINE, IOP_FUNC)

/// Calls `iop_panic` with provided message if condition is false
#define iop_assert(cond, msg)                                                  \
  if (!(cond))                                                                 \
    iop_panic(msg);

#endif