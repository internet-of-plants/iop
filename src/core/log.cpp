#include "core/log.hpp"
#include "core/utils.hpp"
#include <string>
#include "driver/device.hpp"
#include "driver/wifi.hpp"
#include <umm_malloc/umm_heap_select.h>

static bool initialized = false;
static bool isTracing_ = false; 
static bool shouldFlush_ = true;

void iop::Log::shouldFlush(const bool flush) noexcept {
  shouldFlush_ = flush;
}

auto iop::Log::isTracing() noexcept -> bool { 
  return isTracing_;
}

constexpr static iop::LogHook defaultHook(iop::LogHook::defaultViewPrinter,
                                      iop::LogHook::defaultStaticPrinter,
                                      iop::LogHook::defaultSetuper,
                                      iop::LogHook::defaultFlusher);
static iop::LogHook hook = defaultHook;

namespace iop {
void IRAM_ATTR Log::setup(LogLevel level) noexcept { hook.setup(level); }
void Log::flush() noexcept { if (shouldFlush_) hook.flush(); }
void IRAM_ATTR Log::print(const std::string_view view, const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.viewPrint(view, level, kind);
  else
    hook.traceViewPrint(view, level, kind);
}
void IRAM_ATTR Log::print(const StaticString progmem,
                                const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.staticPrint(progmem, level, kind);
  else
    hook.traceStaticPrint(progmem, level, kind);
}
auto Log::takeHook() noexcept -> LogHook {
  initialized = false;
  auto old = hook;
  hook = defaultHook;
  return old;
}
void Log::setHook(LogHook newHook) noexcept {
  initialized = false;
  hook = std::move(newHook);
}

void Log::printLogType(const LogType &logType,
                       const LogLevel &level) const noexcept {
  if (level == LogLevel::NO_LOG)
    return;

  switch (logType) {
  case LogType::CONTINUITY:
  case LogType::END:
    break;

  case LogType::START:
  case LogType::STARTEND:
    Log::print(F("["), level, LogType::START);
    Log::print(this->levelToString(level).get(), level, LogType::CONTINUITY);
    Log::print(F("] "), level, LogType::CONTINUITY);
    Log::print(this->target_.get(), level, LogType::CONTINUITY);
    Log::print(F(": "), level, LogType::CONTINUITY);
  };
}

void Log::log(const LogLevel &level, const StaticString &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level_ > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  Log::print(lineTermination, level, LogType::END);
  Log::flush();
}

void Log::log(const LogLevel &level, const std::string_view &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level_ > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  Log::print(lineTermination, level, LogType::END);
  Log::flush();
}

auto Log::levelToString(const LogLevel level) const noexcept -> StaticString {
  switch (level) {
  case LogLevel::TRACE:
    return F("TRACE");
  case LogLevel::DEBUG:
    return F("DEBUG");
  case LogLevel::INFO:
    return F("INFO");
  case LogLevel::WARN:
    return F("WARN");
  case LogLevel::ERROR:
    return F("ERROR");
  case LogLevel::CRIT:
    return F("CRIT");
  case LogLevel::NO_LOG:
    return F("NO_LOG");
  }
  return F("UNKNOWN");
}

void IRAM_ATTR LogHook::defaultStaticPrinter(
    const StaticString str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IRAM_ATTR
LogHook::defaultViewPrinter(const std::string_view str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IRAM_ATTR
LogHook::defaultSetuper(const LogLevel level) noexcept {
  isTracing_ |= level == LogLevel::TRACE;
  static bool hasInitialized = false;
  const auto shouldInitialize = !hasInitialized;
  hasInitialized = true;
  if (shouldInitialize)
    logSetup(level);
}
void LogHook::defaultFlusher() noexcept {
#ifdef IOP_SERIAL
  logFlush();
#endif
}
// NOLINTNEXTLINE *-use-equals-default
LogHook::LogHook(LogHook const &other) noexcept
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
LogHook::LogHook(LogHook &&other) noexcept
    // NOLINTNEXTLINE cert-oop11-cpp cert-oop54-cpp *-move-constructor-init
    : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
      setup(other.setup), flush(other.flush),
      traceViewPrint(other.traceViewPrint),
      traceStaticPrint(other.traceStaticPrint) {}
auto LogHook::operator=(LogHook const &other) noexcept -> LogHook & {
  if (this == &other)
    return *this;
  this->viewPrint = other.viewPrint;
  this->staticPrint = other.staticPrint;
  this->setup = other.setup;
  this->flush = other.flush;
  this->traceViewPrint = other.traceViewPrint;
  this->traceStaticPrint = other.traceStaticPrint;
  return *this;
}
auto LogHook::operator=(LogHook &&other) noexcept -> LogHook & {
  *this = other;
  return *this;
}

Tracer::Tracer(CodePoint point) noexcept : point(std::move(point)) {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Entering new scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n[TRACE] TRACER: Free Stack "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(driver::device.availableStack()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", Free DRAM "), LogLevel::TRACE, LogType::CONTINUITY);
  {
    HeapSelectDram ephemeral;
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(F(", Biggest DRAM Block "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  {
    HeapSelectIram ephemeral;
    Log::print(F(", Free IRAM "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  Log::print(F(", Connection "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(WiFi.status() == WL_CONNECTED), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Leaving scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

void logMemory(const Log &logger) noexcept {
  IOP_TRACE();
  if (logger.level() > LogLevel::INFO) return;
  Log::flush();
  Log::print(F("[INFO] "), LogLevel::INFO, LogType::START);
  Log::print(logger.target(), LogLevel::INFO, LogType::START);
  Log::print(F(": Free Stack "), LogLevel::INFO, LogType::CONTINUITY);
  Log::print(std::to_string(driver::device.availableStack()), LogLevel::INFO, LogType::CONTINUITY);
  Log::print(F(", Free IRAM "), LogLevel::INFO, LogType::CONTINUITY);
  {
    HeapSelectIram ephemeral;
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::INFO, LogType::CONTINUITY);
    Log::print(F(", Biggest IRAM Block "), LogLevel::INFO, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::INFO, LogType::CONTINUITY);
  }
  Log::print(F(", Free DRAM "), LogLevel::INFO, LogType::CONTINUITY);
  {
    HeapSelectDram ephemeral;
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::INFO, LogType::CONTINUITY);
  }
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

} // namespace iop