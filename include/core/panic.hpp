#ifndef IOP_PANIC_HPP
#define IOP_PANIC_HPP

#include "core/string/view.hpp"
#include "core/tracer.hpp"
#include <functional>

namespace iop {
class PanicHook {
public:
  using ViewPanic = std::function<void(StringView const &, CodePoint const &)>;
  using StaticPanic =
      std::function<void(StaticString const &, CodePoint const &)>;
  using Entry = std::function<void(StringView const &, CodePoint const &)>;
  using Halt = std::function<void(StringView const &, CodePoint const &)>;

  ViewPanic viewPanic;
  StaticPanic staticPanic;
  Entry entry;
  Halt halt;

  static void defaultViewPanic(StringView const &msg,
                               CodePoint const &point) noexcept;
  static void defaultStaticPanic(StaticString const &msg,
                                 CodePoint const &point) noexcept;
  static void defaultEntry(StringView const &msg,
                           CodePoint const &point) noexcept;
  static void defaultHalt(StringView const &msg,
                          CodePoint const &point) noexcept
      __attribute__((noreturn));

  PanicHook(ViewPanic view, StaticPanic progmem, Entry entry,
            Halt halt) noexcept
      : viewPanic(view), staticPanic(progmem), entry(entry), halt(halt) {
    IOP_TRACE();
  }
};

/// Replaces current hook for this. Very useful to support panics that report to
/// the network, write to flash, and try to update. Default just prints to
/// serial
void setPanicHook(PanicHook hook) noexcept;
/// Removes current hook, replaces for default one (that just prints to
/// Serial)
auto takePanicHook() noexcept -> PanicHook;

void panicHandler(StringView msg, CodePoint const &point) noexcept
    __attribute__((noreturn));
void panicHandler(StaticString msg, CodePoint const &point) noexcept
    __attribute__((noreturn));
} // namespace iop

/// Panic. Never returns. Logs panic to network if available (and serial).
/// If any update is available it's installed and reboots.
///
/// Sent: Message + Line + Function + File
#define iop_panic(msg) iop::panicHandler((msg), IOP_CODE_POINT())

/// Calls `iop_panic` with provided message if condition is false
#define iop_assert(cond, msg)                                                  \
  if (!(cond))                                                                 \
    iop_panic(msg);

#endif