#include "driver/panic.hpp"
#include "driver/log.hpp"
#include "driver/device.hpp"
#include "driver/thread.hpp"

static bool isPanicking = false;

constexpr static iop::PanicHook defaultHook(iop::PanicHook::defaultViewPanic,
                                        iop::PanicHook::defaultStaticPanic,
                                        iop::PanicHook::defaultEntry,
                                        iop::PanicHook::defaultHalt);

static iop::PanicHook hook(defaultHook);

namespace iop {
Log & panicLogger() noexcept {
  static iop::Log logger(iop::LogLevel::CRIT, IOP_STATIC_STRING("PANIC"));
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
  iop::panicLogger().crit(IOP_STATIC_STRING("Line "), ::std::to_string(point.line()), IOP_STATIC_STRING(" of file "), point.file(),
              IOP_STATIC_STRING(" inside "), point.func(), IOP_STATIC_STRING(": "), msg);
}
void PanicHook::defaultStaticPanic(iop::StaticString const &msg,
                                   CodePoint const &point) noexcept {
  iop::panicLogger().crit(IOP_STATIC_STRING("Line "), ::std::to_string(point.line()), IOP_STATIC_STRING(" of file "), point.file(),
              IOP_STATIC_STRING(" inside "), point.func(), IOP_STATIC_STRING(": "), msg);
}
void PanicHook::defaultEntry(std::string_view const &msg,
                             CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    iop::panicLogger().crit(IOP_STATIC_STRING("PANICK REENTRY: Line "), std::to_string(point.line()),
                IOP_STATIC_STRING(" of file "), point.file(), IOP_STATIC_STRING(" inside "), point.func(),
                IOP_STATIC_STRING(": "), msg);
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