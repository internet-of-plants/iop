#include "core/panic.hpp"
#include "core/log.hpp"
#include "driver/device.hpp"
#include "driver/thread.hpp"
#include <string>

static bool isPanicking = false;

constexpr static iop::PanicHook defaultHook(iop::PanicHook::defaultViewPanic,
                                        iop::PanicHook::defaultStaticPanic,
                                        iop::PanicHook::defaultEntry,
                                        iop::PanicHook::defaultHalt);

static iop::PanicHook hook(defaultHook);

namespace iop {
Log & panicLogger() noexcept {
  static iop::Log logger(iop::LogLevel::CRIT, FLASH("PANIC"));
  return logger;
}

void panicHandler(std::string_view msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  hook.entry(msg, point);
  hook.viewPanic(msg, point);
  hook.halt(msg, point);
  driver::thisThread.panic_();
}

void panicHandler(StaticString msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  const auto msg_ = msg.toString();
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  driver::thisThread.panic_();
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
void setPanicHook(PanicHook newHook) noexcept { hook = std::move(newHook); }

void PanicHook::defaultViewPanic(std::string_view const &msg,
                                 CodePoint const &point) noexcept {
  iop::panicLogger().crit(FLASH("Line "), ::std::to_string(point.line()), FLASH(" of file "), point.file(),
              FLASH(" inside "), point.func(), FLASH(": "), msg);
}
void PanicHook::defaultStaticPanic(iop::StaticString const &msg,
                                   CodePoint const &point) noexcept {
  iop::panicLogger().crit(FLASH("Line "), ::std::to_string(point.line()), FLASH(" of file "), point.file(),
              FLASH(" inside "), point.func(), FLASH(": "), msg);
}
void PanicHook::defaultEntry(std::string_view const &msg,
                             CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    iop::panicLogger().crit(FLASH("PANICK REENTRY: Line "), std::to_string(point.line()),
                FLASH(" of file "), point.file(), FLASH(" inside "), point.func(),
                FLASH(": "), msg);
    iop::logMemory(iop::panicLogger());
    driver::device.deepSleep(0);
    driver::thisThread.panic_();
  }
  isPanicking = true;

  constexpr const uint16_t oneSecond = 1000;
  driver::thisThread.sleep(oneSecond);
}
void PanicHook::defaultHalt(std::string_view const &msg,
                            CodePoint const &point) noexcept {
  (void)msg;
  (void)point;
  IOP_TRACE();
  driver::device.deepSleep(0);
  driver::thisThread.panic_();
}
} // namespace iop