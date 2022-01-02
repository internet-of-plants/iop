#ifndef IOP_SERIAL
#include "driver/noop/log.hpp"
#elif defined(IOP_DESKTOP)
#include "driver/desktop/log.hpp"
#elif defined(IOP_ESP8266)
#include "driver/esp8266/log.hpp"
#elif defined(IOP_NOOP)
#ifdef ARDUINO
#include "driver/esp8266/log.hpp"
#else
#include "driver/desktop/log.hpp"
#endif
#elif defined(IOP_ESP32)
#include "driver/esp32/log.hpp"
#else
#error "Target not supported"
#endif

#include "driver/network.hpp"
#include "driver/device.hpp"
#include "driver/wifi.hpp"

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
void IOP_RAM Log::setup(LogLevel level) noexcept { hook.setup(level); }
void Log::flush() noexcept { if (shouldFlush_) hook.flush(); }
void IOP_RAM Log::print(const iop::StringView view, const LogLevel level,
                                const LogType kind) noexcept {
  if (level > LogLevel::TRACE)
    hook.viewPrint(view, level, kind);
  else
    hook.traceViewPrint(view, level, kind);
}
void IOP_RAM Log::print(const StaticString progmem,
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
    Log::print(IOP_STATIC_STRING("["), level, LogType::START);
    Log::print(this->levelToString(level).get(), level, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING("] "), level, LogType::CONTINUITY);
    Log::print(this->target_.get(), level, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING(": "), level, LogType::CONTINUITY);
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
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

void Log::log(const LogLevel &level, const iop::StringView &msg,
              const LogType &logType,
              const StaticString &lineTermination) const noexcept {
  if (this->level_ > level)
    return;

  Log::flush();
  this->printLogType(logType, level);
  Log::print(msg, level, LogType::CONTINUITY);
  if (logType == LogType::END || logType == LogType::STARTEND) {
    Log::print(lineTermination, level, LogType::END);
  } else if (lineTermination.length() != 0) {
    Log::print(lineTermination, level, LogType::CONTINUITY);
  }
  Log::flush();
}

auto Log::levelToString(const LogLevel level) const noexcept -> StaticString {
  switch (level) {
  case LogLevel::TRACE:
    return IOP_STATIC_STRING("TRACE");
  case LogLevel::DEBUG:
    return IOP_STATIC_STRING("DEBUG");
  case LogLevel::INFO:
    return IOP_STATIC_STRING("INFO");
  case LogLevel::WARN:
    return IOP_STATIC_STRING("WARN");
  case LogLevel::ERROR:
    return IOP_STATIC_STRING("ERROR");
  case LogLevel::CRIT:
    return IOP_STATIC_STRING("CRIT");
  case LogLevel::NO_LOG:
    return IOP_STATIC_STRING("NO_LOG");
  }
  return IOP_STATIC_STRING("UNKNOWN");
}

void IOP_RAM LogHook::defaultStaticPrinter(
    const StaticString str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  driver::logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultViewPrinter(const iop::StringView str, const LogLevel level, const LogType type) noexcept {
#ifdef IOP_SERIAL
  driver::logPrint(str);
#else
  (void)str;
#endif
  (void)type;
  (void)level;
}
void IOP_RAM
LogHook::defaultSetuper(const LogLevel level) noexcept {
  isTracing_ |= level == LogLevel::TRACE;
  static bool hasInitialized = false;
  const auto shouldInitialize = !hasInitialized;
  hasInitialized = true;
  if (shouldInitialize)
    driver::logSetup(level);
}
void LogHook::defaultFlusher() noexcept {
#ifdef IOP_SERIAL
  driver::logFlush();
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
  Log::print(IOP_STATIC_STRING("[TRACE] TRACER: Entering new scope, at line "), LogLevel::TRACE, LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING("\n[TRACE] TRACER: Free Stack "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(driver::device.availableStack()), LogLevel::TRACE, LogType::CONTINUITY);
  {
    driver::HeapSelectDram guard;
    Log::print(IOP_STATIC_STRING(", Free DRAM "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING(", Biggest DRAM Block "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  {
    driver::HeapSelectIram guard;
    Log::print(IOP_STATIC_STRING(", Free IRAM "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING(", Biggest IRAM Block "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  Log::print(IOP_STATIC_STRING(", Connection "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(iop::data.wifi.status() == driver::StationStatus::GOT_IP), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(IOP_STATIC_STRING("[TRACE] TRACER: Leaving scope, at line "), LogLevel::TRACE, LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

void logMemory(const Log &logger) noexcept {
  if (logger.level() > LogLevel::INFO) return;

  Log::flush();
  Log::print(IOP_STATIC_STRING("[INFO] "), logger.level(), LogType::START);
  Log::print(logger.target(), logger.level(), LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING(": Free Stack "), logger.level(), LogType::CONTINUITY);
  Log::print(std::to_string(driver::device.availableStack()), LogLevel::TRACE, LogType::CONTINUITY);
  {
    driver::HeapSelectDram guard;
    Log::print(IOP_STATIC_STRING(", Free DRAM "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING(", Biggest DRAM Block "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  {
    driver::HeapSelectIram guard;
    Log::print(IOP_STATIC_STRING(", Free IRAM "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.availableHeap()), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(IOP_STATIC_STRING(", Biggest IRAM Block "), LogLevel::TRACE, LogType::CONTINUITY);
    Log::print(std::to_string(driver::device.biggestHeapBlock()), LogLevel::TRACE, LogType::CONTINUITY);
  }
  Log::print(IOP_STATIC_STRING(", Connection "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(iop::data.wifi.status() == driver::StationStatus::GOT_IP), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(IOP_STATIC_STRING("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
} // namespace iop