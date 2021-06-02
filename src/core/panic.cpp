#include "core/panic.hpp"
#include "core/log.hpp"
#include "core/tracer.hpp"
#include "core/utils.hpp"

#include <iostream>

#include "driver/device.hpp"

static auto panicTarget() -> iop::StaticString {
  return iop::StaticString(F("PANIC"));
}
const static iop::Log logger(iop::LogLevel::CRIT, panicTarget());

static bool isPanicking = false;

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
  std::abort();
  while (true) {
  } // Is this UB? It will trigger software watch-dog, but shouldn't be reached
}

#include <iostream>
void panicHandler(StaticString msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  const auto msg_ = msg.toStdString();
  hook.entry(msg_, point);
  hook.staticPanic(msg, point);
  hook.halt(msg_, point);
  std::abort();
  while (true) {
  } // Is this UB? It will trigger software watch-dog, but shouldn't be reached
}
auto takePanicHook() noexcept -> PanicHook {
  auto old = hook;
  hook = defaultHook;
  return old;
}
void setPanicHook(PanicHook newHook) noexcept { hook = std::move(newHook); }

void PanicHook::defaultViewPanic(StringView const &msg,
                                 CodePoint const &point) noexcept {
  logger.crit(F("Line "), ::std::to_string(point.line()), F(" of file "), point.file(),
              F(" inside "), point.func(), F(": "), msg);
}
void PanicHook::defaultStaticPanic(StaticString const &msg,
                                   CodePoint const &point) noexcept {
  logger.crit(F("Line "), ::std::to_string(point.line()), F(" of file "), point.file(),
              F(" inside "), point.func(), F(": "), msg);
}
void PanicHook::defaultEntry(StringView const &msg,
                             CodePoint const &point) noexcept {
  IOP_TRACE();
  if (isPanicking) {
    logger.crit(F("PANICK REENTRY: Line "), std::to_string(point.line()),
                F(" of file "), point.file(), F(" inside "), point.func(),
                F(": "), msg);
    iop::logMemory(logger);
    ESP.deepSleep(0);
    __panic_func(point.file().asCharPtr(), point.line(), point.func().get());
  }
  isPanicking = true;

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