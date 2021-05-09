#include "core/panic.hpp"
#include "core/log.hpp"
#include "core/tracer.hpp"
#include "core/utils.hpp"

#include <iostream>


#ifdef IOP_DESKTOP
#include <thread>
#include <cstdlib>
#include <chrono>
class Esp {
public:
  void deepSleep(uint32_t microsecs) {
    std::this_thread::sleep_for(std::chrono::microseconds(microsecs));
  }
};
static Esp ESP;
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
void __panic_func(const char *file, uint16_t line, const char *func) noexcept __attribute__((noreturn));
void __panic_func(const char *file, uint16_t line, const char *func) noexcept {
  std::cout << file << " " << line << " " << func << std::endl;
  std::abort();
}
void delay(uint32_t time) {
  std::this_thread::sleep_for(std::chrono::milliseconds(time));
}
#else
#include "Esp.h"
#endif

PROGMEM_STRING(loggingTarget, "PANIC")
const static iop::Log logger(iop::LogLevel::CRIT, loggingTarget);
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
  while (true) {
  } // Will trigger software watch-dog, but shouldn't be reached
}

void panicHandler(StaticString msg, CodePoint const &point) noexcept {
  IOP_TRACE();
  const auto msg_ = msg.toStdString();
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
  ::std::cout << "Line " << ::std::to_string(point.line()) << " of file " << point.file().asCharPtr() << " inside " << point.func().get() << ": " << msg.get() << std::endl;
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