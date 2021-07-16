#ifndef IOP_CORE_LOG_HPP
#define IOP_CORE_LOG_HPP

#include "driver/log.hpp"
#include <functional>

#define IOP_FILE ::iop::StaticString(FPSTR(__FILE__))
#define IOP_LINE static_cast<uint32_t>(__LINE__)
#define IOP_FUNC ::iop::StaticString(FPSTR(__PRETTY_FUNCTION__))

/// Returns CodePoint object pointing to the caller
/// this is useful to track callers of functions that can panic
#define IOP_CTX() IOP_CODE_POINT()
#define IOP_CODE_POINT() ::iop::CodePoint(IOP_FILE, IOP_LINE, IOP_FUNC)

/// Logs scope changes to serial if logLevel is set to TRACE
#define IOP_TRACE() IOP_TRACE_INNER(__COUNTER__)
// Technobabble to stringify __COUNTER__
#define IOP_TRACE_INNER(x) IOP_TRACE_INNER2(x)
#define IOP_TRACE_INNER2(x) const ::iop::Tracer iop_tracer_##x(IOP_CODE_POINT());

namespace iop {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };
enum class LogType { START, CONTINUITY, STARTEND, END };

class LogHook {
public:
  using ViewPrinter = void (*) (std::string_view, LogLevel, LogType);
  using StaticPrinter = void (*) (StaticString, LogLevel, LogType);
  using Setuper = void (*) (LogLevel);
  using Flusher = void (*) ();

  using TraceViewPrinter = ViewPrinter;
  using TraceStaticPrinter = StaticPrinter;

  ViewPrinter viewPrint;
  StaticPrinter staticPrint;
  Setuper setup;
  Flusher flush;
  TraceViewPrinter traceViewPrint;
  TraceStaticPrinter traceStaticPrint;

  /// Prints log to Serial.
  /// May be called from interrupt because it's the default tracing printer
  static void defaultViewPrinter(std::string_view, LogLevel level, LogType type) noexcept;
  /// Prints log to Serial.
  /// May be called from interrupt because it's the default tracing printer
  static void defaultStaticPrinter(StaticString, LogLevel level,
                                   LogType type) noexcept;
  static void defaultSetuper(iop::LogLevel level) noexcept;
  static void defaultFlusher() noexcept;

  constexpr LogHook(LogHook::ViewPrinter viewPrinter,
                 LogHook::StaticPrinter staticPrinter, LogHook::Setuper setuper,
                 LogHook::Flusher flusher) noexcept
    : viewPrint(std::move(viewPrinter)), staticPrint(std::move(staticPrinter)),
      setup(std::move(setuper)), flush(std::move(flusher)),
      traceViewPrint(defaultViewPrinter),
      traceStaticPrint(defaultStaticPrinter) {}

  // Specifies custom tracer funcs, may be called from interrupts (put it into
  // ICACHE_RAM). Don't be fancy, and be aware, it can saturate channels very
  // fast
  constexpr LogHook(LogHook::ViewPrinter viewPrinter,
                  LogHook::StaticPrinter staticPrinter, LogHook::Setuper setuper,
                  LogHook::Flusher flusher,
                  LogHook::TraceViewPrinter traceViewPrint,
                  LogHook::TraceStaticPrinter traceStaticPrint) noexcept
      : viewPrint(std::move(viewPrinter)), staticPrint(std::move(staticPrinter)),
        setup(std::move(setuper)), flush(std::move(flusher)),
        traceViewPrint(std::move(traceViewPrint)),
        traceStaticPrint(std::move(traceStaticPrint)) {}
  ~LogHook() noexcept = default;
  LogHook(LogHook const &other) noexcept;
  LogHook(LogHook &&other) noexcept;
  auto operator=(LogHook const &other) noexcept -> LogHook &;
  auto operator=(LogHook &&other) noexcept -> LogHook &;
};

static auto defaultLineTermination() -> StaticString {
  return StaticString(F("\n"));
}

/// Logger with its own log level and target
class Log {
  LogLevel level_;
  StaticString target_;

public:
  Log(const LogLevel &level, StaticString target) noexcept
      : level_{level}, target_(std::move(target)) {}

  /// Replaces current hook for this. Very useful to support other logging
  /// channels, like network or flash. Default just prints to serial.
  ///
  /// The default logger (Serial) already will be initialized, call Serial.end()
  /// in your setuper if you don't want Serial to be initialized. Or undef
  /// `IOP_SERIAL` to make the default loggers into noops.
  static void setHook(LogHook hook) noexcept;

  /// Removes current hook, replaces for default one (that just prints to
  /// Serial)
  static auto takeHook() noexcept -> LogHook;

  auto level() const noexcept -> LogLevel { return this->level_; }
  auto target() const noexcept -> StaticString { return this->target_; }
  static void shouldFlush(bool flush) noexcept;
  static auto isTracing() noexcept -> bool;

  template <typename... Args> void trace(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::TRACE, true, args...);
  }
  template <typename... Args> void debug(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::DEBUG, true, args...);
  }
  template <typename... Args> void info(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::INFO, true, args...);
  }
  template <typename... Args> void warn(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::WARN, true, args...);
  }
  template <typename... Args> void error(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::ERROR, true, args...);
  }
  template <typename... Args> void crit(const Args &...args) const noexcept {
    this->log_recursive(LogLevel::CRIT, true, args...);
  }

  static void print(StaticString progmem, LogLevel level, LogType kind) noexcept;
  static void print(std::string_view view, LogLevel level, LogType kind) noexcept;
  static void flush() noexcept;
  static void setup(LogLevel level) noexcept;

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, F(""));
    } else {
      this->log(level, msg, LogType::CONTINUITY, F(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, F("\n"));
    } else {
      this->log(level, msg, LogType::END, F("\n"));
    }
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const std::string_view msg, const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, F(""));
    } else {
      this->log(level, msg, LogType::CONTINUITY, F(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const std::string_view msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, F("\n"));
    } else {
      this->log(level, msg, LogType::END, F("\n"));
    }
  }

  void printLogType(const LogType &logType, const LogLevel &level) const noexcept;
  auto levelToString(LogLevel level) const noexcept -> StaticString;

  void log(const LogLevel &level, const StaticString &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;
  void log(const LogLevel &level, const std::string_view &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;
};

class CodePoint {
  StaticString file_;
  uint32_t line_;
  StaticString func_;

public:
  // Use the IOP_CODE_POINT() macro to construct CodePoint
  CodePoint(StaticString file, uint32_t line, StaticString func) noexcept
      : file_(file), line_(line), func_(func) {}
  auto file() const noexcept -> StaticString { return this->file_; }
  auto line() const noexcept -> uint32_t { return this->line_; }
  auto func() const noexcept -> StaticString { return this->func_; }
};

/// Tracer objects, that signifies scoping changes. Helps with post-mortemns
/// analysis
///
/// Doesn't use the official logging system because it's supposed to be used
/// only when physically debugging very specific bugs, and is unfit for network
/// logging.
class Tracer {
  CodePoint point;

public:
  explicit Tracer(CodePoint point) noexcept;
  ~Tracer() noexcept;
  Tracer(const Tracer &other) noexcept = delete;
  Tracer(Tracer &&other) noexcept = delete;
  auto operator=(const Tracer &other) noexcept -> Tracer & = delete;
  auto operator=(Tracer &&other) noexcept -> Tracer & = delete;
};
} // namespace iop

#endif
