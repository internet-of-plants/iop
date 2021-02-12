#include "core/panic.hpp"
#include "core/log.hpp"
#include "core/tracer.hpp"
#include "core/utils.hpp"

#include "Esp.h"

PROGMEM_STRING(logTarget, "PANIC")
const static iop::Log logger(iop::LogLevel::CRIT, logTarget);
static bool panicking = false;

const static iop::PanicHook defaultHook(iop::PanicHook::defaultViewPanic,
                                        iop::PanicHook::defaultStaticPanic,
                                        iop::PanicHook::defaultEntry,
                                        iop::PanicHook::defaultHalt);

static iop::PanicHook hook(defaultHook);

namespace iop {
void panicHandler(StringView msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  hook.entry(msg, point);
  hook.viewPanic(msg, point);
  hook.halt(msg, point);
  while (true) {
  } // Will trigger software watch-dog, but shouldn't be reached
}

void panicHandler(StaticString msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  String msg_(msg.get());
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  while (true) {
  } // Will trigger software watch-dog, but shouldn't be reached
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
void setPanicHook(PanicHook newHook) noexcept { hook = std::move(newHook); }

void PanicHook::defaultViewPanic(StringView const &msg,
                                 CodePoint const &point) noexcept {
  logger.crit(F("Line "), String(point.line()), F(" of file "), point.file(),
              F(" inside "), point.func(), F(": "), msg);
}
void PanicHook::defaultStaticPanic(StaticString const &msg,
                                   CodePoint const &point) noexcept {
  logger.crit(F("Line "), String(point.line()), F(" of file "), point.file(),
              F(" inside "), point.func(), F(": "), msg);
}
void PanicHook::defaultEntry(StringView const &msg,
                             CodePoint const &point) noexcept {
  IOP_TRACE();
  if (panicking) {
    logger.crit(F("PANICK REENTRY: Line "), String(point.line()),
                F(" of file "), point.file(), F(" inside "), point.func(),
                F(": "), msg);
    iop::logMemory(logger);
    ESP.deepSleep(0);
    __panic_func(point.file().asCharPtr(), point.line(), point.func().get());
  }
  panicking = true;

  constexpr const uint16_t oneSecond = 1000;
  delay(oneSecond);
}
void PanicHook::defaultHalt(StringView const &msg,
                            CodePoint const &point) noexcept {
  (void)msg;
  IOP_TRACE();
  ESP.deepSleep(0);
  __panic_func(point.file().asCharPtr(), point.line(), point.func().get());
}
} // namespace iop