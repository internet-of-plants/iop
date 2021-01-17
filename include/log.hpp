#ifndef IOP_LOG_HPP
#define IOP_LOG_HPP

#include "option.hpp"
#include "static_string.hpp"
#include "string_view.hpp"

// TODO: think about logging some important things to flash (FS.h)

class Api;

enum LogLevel { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };

enum LogType { CONTINUITY, START };

PROGMEM_STRING(defaultLineTermination, "\n");

/// Logger with its own log level and target
class Log {
private:
  LogLevel logLevel;
  StaticString targetLogger;
  bool flush = true;

  void printLogType(enum LogType logType, LogLevel level) const noexcept;

  void log(LogLevel level, StaticString msg, enum LogType logType,
           StaticString lineTermination) const noexcept;
  void log(LogLevel level, StringView msg, enum LogType logType,
           StaticString lineTermination) const noexcept;

  static void print(StaticString str) noexcept;
  static void print(StringView str) noexcept;
  static void reportLog() noexcept;

public:
  ~Log() = default;
  Log(LogLevel level, StaticString target) noexcept
      : logLevel{level}, targetLogger(std::move(target)) {}
  Log(LogLevel level, StaticString target, bool flush) noexcept
      : logLevel{level}, targetLogger(std::move(target)), flush{flush} {}
  Log(Log const &other) = default;
  Log(Log &&other) = default;
  auto operator=(Log const &other) noexcept -> Log & = default;
  auto operator=(Log &&other) noexcept -> Log & = default;
  auto level() const noexcept -> LogLevel { return this->logLevel; }
  auto target() const noexcept -> StaticString { return this->targetLogger; }
  void setup() const noexcept;

  template <typename... Args> void trace(const Args &...args) const noexcept {
    this->log_recursive(TRACE, true, args...);
  }
  template <typename... Args> void debug(const Args &...args) const noexcept {
    this->log_recursive(DEBUG, true, args...);
  }
  template <typename... Args> void info(const Args &...args) const noexcept {
    this->log_recursive(INFO, true, args...);
  }
  template <typename... Args> void warn(const Args &...args) const noexcept {
    this->log_recursive(WARN, true, args...);
  }
  template <typename... Args> void error(const Args &...args) const noexcept {
    this->log_recursive(ERROR, true, args...);
  }
  template <typename... Args> void crit(const Args &...args) const noexcept {
    this->log_recursive(CRIT, true, args...);
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel level, const bool first,
                     const StaticString msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, START, F(""));
    } else {
      this->log(level, msg, CONTINUITY, F(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel level, const bool first,
                     const StaticString msg) const noexcept {
    if (first) {
      this->log(level, msg, START, F("\n"));
    } else {
      this->log(level, msg, CONTINUITY, F("\n"));
    }
    this->reportLog();
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const LogLevel level, const bool first,
                     const StringView msg, const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, START, F(""));
    } else {
      this->log(level, msg, CONTINUITY, F(""));
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel level, const bool first,
                     const StringView msg) const noexcept {
    if (first) {
      this->log(level, msg, START, F("\n"));
    } else {
      this->log(level, msg, CONTINUITY, F("\n"));
    }
    this->reportLog();
  }
};

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_LOG_DISABLED
#endif

#endif
