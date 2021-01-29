#ifndef IOP_LOG_HPP
#define IOP_LOG_HPP

#include "certificate_storage.hpp"

#include "option.hpp"
#include "panic.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include <cstdint>

// TODO: think about logging some important things to flash (FS.h)

class Api;

enum class LogLevel { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };

enum LogType { START = 0, CONTINUITY };

PROGMEM_STRING(defaultLineTermination, "\n");
PROGMEM_STRING(emptyStaticString, "");

/// Logger with its own log level and target
class Log {
private:
  LogLevel logLevel_;
  StaticString targetLogger;
  bool flush = true;

  void printLogType(const LogType &logType,
                    const LogLevel &level) const noexcept;

  void log(const LogLevel &level, const StaticString &msg,
           const LogType &logType,
           const StaticString &lineTermination) const noexcept;
  void log(const LogLevel &level, const StringView &msg, const LogType &logType,
           const StaticString &lineTermination) const noexcept;

  static void print(const __FlashStringHelper *str) noexcept;
  static void print(const char *str) noexcept;
  static void reportLog() noexcept;

public:
  ~Log() = default;
  Log(const LogLevel &level, StaticString target) noexcept
      : logLevel_{level}, targetLogger(std::move(target)) {}
  Log(const LogLevel &level, StaticString target, bool flush) noexcept
      : logLevel_{level}, targetLogger(std::move(target)), flush{flush} {}
  Log(Log const &other) noexcept = default;
  Log(Log &&other) noexcept = default;
  auto operator=(Log const &other) noexcept -> Log & = default;
  auto operator=(Log &&other) noexcept -> Log & = default;
  auto level() const noexcept -> LogLevel { return this->logLevel_; }
  auto target() const noexcept -> StaticString { return this->targetLogger; }
  static void setup() noexcept;

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
      this->log(level, msg, LogType::START, defaultLineTermination);
    } else {
      this->log(level, msg, LogType::CONTINUITY, defaultLineTermination);
    }
    this->reportLog();
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
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const LogLevel &level, const bool first,
                     const StringView msg) const noexcept {
    if (first) {
      this->log(level, msg, LogType::START, defaultLineTermination);
    } else {
      this->log(level, msg, LogType::CONTINUITY, defaultLineTermination);
    }
    this->reportLog();
  }
};

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_LOG_DISABLED
#endif

#endif
