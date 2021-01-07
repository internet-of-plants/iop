#ifndef IOP_LOG_H
#define IOP_LOG_H

#include "option.hpp"
#include "static_string.hpp"
#include "string_view.hpp"

// TODO: think about remote logging (at least errors/crits) - syslog?
// TODO: think about logging some important things to flash (FS.h)

enum LogLevel { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRIT };

enum LogType { CONTINUITY, START };

PROGMEM_STRING(defaultLineTermination, "\n");

/// Logger with its own log level and target
class Log {
private:
  LogLevel logLevel;
  StaticString targetLogger;
  bool flush = true;

  void printLogType(const enum LogType logType,
                    const LogLevel level) const noexcept;

  void log(const LogLevel level, const StaticString msg,
           const enum LogType logType,
           const StaticString lineTermination) const noexcept;
  void log(const LogLevel level, const StringView msg,
           const enum LogType logType,
           const StaticString lineTermination) const noexcept;

public:
  Log(const LogLevel level, const StaticString target) noexcept
      : logLevel{level}, targetLogger(target) {}
  Log(const LogLevel level, const StaticString target,
      const bool flush) noexcept
      : logLevel{level}, targetLogger(target), flush{flush} {}
  Log(Log &other) noexcept
      : logLevel(other.logLevel),
        targetLogger(other.targetLogger), flush{other.flush} {}
  Log(Log &&other) noexcept
      : logLevel(other.logLevel),
        targetLogger(other.targetLogger), flush{other.flush} {}
  Log &operator=(Log &other) noexcept {
    this->logLevel = other.logLevel;
    this->targetLogger = other.targetLogger;
    this->flush = other.flush;
    return *this;
  }
  Log &operator=(Log &&other) noexcept {
    this->logLevel = other.logLevel;
    this->targetLogger = other.targetLogger;
    this->flush = other.flush;
    return *this;
  }
  LogLevel level() const noexcept { return this->logLevel; }
  StaticString target() const noexcept { return this->targetLogger; }
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
  }
};

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_LOG_DISABLED
#endif

#endif
