#ifndef IOP_CORE_LOG_HPP
#define IOP_CORE_LOG_HPP

#include "core/string/view.hpp"
#include <functional>

namespace iop {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };
enum class LogType { START, CONTINUITY, STARTEND, END };

class LogHook {
public:
  using ViewPrinter = std::function<void(const char *, LogType)>;
  using StaticPrinter =
      std::function<void(const __FlashStringHelper *, LogType)>;
  using Setuper = std::function<void(LogLevel)>;
  using Flusher = std::function<void()>;

  using TraceViewPrinter = ViewPrinter;
  using TraceStaticPrinter = StaticPrinter;

  ViewPrinter viewPrint;
  StaticPrinter staticPrint;
  Setuper setup;
  Flusher flush;
  TraceViewPrinter traceViewPrint;
  TraceStaticPrinter traceStaticPrint;

  static void defaultViewPrinter(const char *str, LogType type) noexcept;
  static void defaultStaticPrinter(const __FlashStringHelper *str,
                                   LogType type) noexcept;
  static void defaultSetuper(const iop::LogLevel level) noexcept;
  static void defaultFlusher() noexcept;

  LogHook(ViewPrinter viewPrinter, StaticPrinter staticPrinter, Setuper setuper,
          Flusher flusher) noexcept
      : viewPrint(std::move(viewPrinter)),
        staticPrint(std::move(staticPrinter)), setup(std::move(setuper)),
        flush(std::move(flusher)), traceViewPrint(defaultViewPrinter),
        traceStaticPrint(defaultStaticPrinter) {}
  // Specifies custom tracer funcs, may be called from interrupts. Don't be
  // fancy, and be ware, it can saturate channels very fast
  LogHook(ViewPrinter viewPrinter, StaticPrinter staticPrinter, Setuper setuper,
          Flusher flusher, TraceViewPrinter traceViewPrint,
          TraceStaticPrinter traceStaticPrint) noexcept
      : viewPrint(std::move(viewPrinter)),
        staticPrint(std::move(staticPrinter)), setup(std::move(setuper)),
        flush(std::move(flusher)), traceViewPrint(traceViewPrint),
        traceStaticPrint(traceStaticPrint) {}
  ~LogHook() noexcept = default;
  LogHook(LogHook const &other) noexcept
      : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
        setup(other.setup), flush(other.flush) {}
  LogHook(LogHook &&other) noexcept
      : viewPrint(other.viewPrint), staticPrint(other.staticPrint),
        setup(other.setup), flush(other.flush) {}
  auto operator=(LogHook const &other) noexcept -> LogHook & {
    this->viewPrint = other.viewPrint;
    this->staticPrint = other.staticPrint;
    this->flush = other.flush;
    this->setup = other.setup;
    return *this;
  }
  auto operator=(LogHook &&other) noexcept -> LogHook & {
    this->viewPrint = other.viewPrint;
    this->staticPrint = other.staticPrint;
    this->flush = other.flush;
    this->setup = other.setup;
    return *this;
  }
};

PROGMEM_STRING(defaultLineTermination, "\n");

/// Logger with its own log level and target
class Log {
  LogLevel level_;
  StaticString target_;

public:
  Log(const LogLevel &level, StaticString target) noexcept
      : level_{level}, target_(std::move(target)) {}

  /// Replaces current hook for this. Very useful to support other logging
  /// channels, like network or flash. Default just prints to serial
  ///
  /// The default logger (Serial) already will be initialized, call Serial.end()
  /// in your setuper if you don't want Serial to be initialized. TODO: add and
  /// `end` to LogHook?
  static void setHook(LogHook hook) noexcept;

  /// Removes current hook, replaces for default one (that just prints to
  /// Serial)
  static auto takeHook() noexcept -> LogHook;

  auto level() const noexcept -> LogLevel { return this->level_; }
  auto target() const noexcept -> StaticString { return this->target_; }
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

  static void print(const __FlashStringHelper *progmem, LogLevel level,
                    LogType kind) noexcept;
  static void print(const char *view, LogLevel level, LogType kind) noexcept;
  static void flush() noexcept;
  static void setup(LogLevel level) noexcept;

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, emptyStaticString);
    } else {
      this->log(level, msg, LogType::CONTINUITY, emptyStaticString);
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StaticString msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, defaultLineTermination);
    } else {
      this->log(level, msg, LogType::END, defaultLineTermination);
    }
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StringView msg, const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, emptyStaticString);
    } else {
      this->log(level, msg, LogType::CONTINUITY, emptyStaticString);
    }
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StringView msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::STARTEND, defaultLineTermination);
    } else {
      this->log(level, msg, LogType::END, defaultLineTermination);
    }
  }

  void printLogType(const LogType &logType,
                    const LogLevel &level) const noexcept;

  void log(const LogLevel &level, const StaticString &msg,
           const LogType &logType,
           const StaticString &lineTermination) const noexcept;
  void log(const LogLevel &level, const StringView &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;

  auto levelToString() const noexcept -> StaticString {
    switch (this->level()) {
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

public:
  ~Log() = default;
  Log(Log const &other) noexcept = default;
  Log(Log &&other) noexcept = default;
  auto operator=(Log const &other) noexcept -> Log & = default;
  auto operator=(Log &&other) noexcept -> Log & = default;
};
} // namespace iop

#endif