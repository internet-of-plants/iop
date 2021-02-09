#ifndef IOP_LOG_HPP
#define IOP_LOG_HPP

#include "core/log.hpp"
#include "core/string/view.hpp"

// TODO: think about logging some important things to flash (FS.h)

class Api;

PROGMEM_STRING(defaultLineTermination, "\n");
PROGMEM_STRING(emptyStaticString, "");

/// Logger with its own log level and target
class Log {
private:
  iop::LogLevel logLevel_;
  iop::StaticString targetLogger;

public:
  Log(const iop::LogLevel &level, iop::StaticString target) noexcept
      : logLevel_{level}, targetLogger(std::move(target)) {}
  auto level() const noexcept -> iop::LogLevel { return this->logLevel_; }
  auto target() const noexcept -> iop::StaticString {
    return this->targetLogger;
  }
  static void setup() noexcept;

  template <typename... Args> void trace(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::TRACE, true, args...);
  }
  template <typename... Args> void debug(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::DEBUG, true, args...);
  }
  template <typename... Args> void info(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::INFO, true, args...);
  }
  template <typename... Args> void warn(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::WARN, true, args...);
  }
  template <typename... Args> void error(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::ERROR, true, args...);
  }
  template <typename... Args> void crit(const Args &...args) const noexcept {
    this->log_recursive(iop::LogLevel::CRIT, true, args...);
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const iop::LogLevel &level, const bool first,
                     const iop::StaticString msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, iop::LogType::START, emptyStaticString);
    } else {
      this->log(level, msg, iop::LogType::CONTINUITY, emptyStaticString);
    }
    this->log_recursive(level, false, args...);
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const iop::LogLevel &level, const bool first,
                     const iop::StaticString msg) const noexcept {
    if (first) {
      this->log(level, msg, iop::LogType::STARTEND, defaultLineTermination);
    } else {
      this->log(level, msg, iop::LogType::END, defaultLineTermination);
    }
  }

  // "Recursive" variadic function
  template <typename... Args>
  void log_recursive(const iop::LogLevel &level, const bool first,
                     const iop::StringView msg,
                     const Args &...args) const noexcept {
    if (first) {
      this->log(level, msg, iop::LogType::START, emptyStaticString);
    } else {
      this->log(level, msg, iop::LogType::CONTINUITY, emptyStaticString);
    }
  }

  // Terminator
  template <typename... Args>
  void log_recursive(const iop::LogLevel &level, const bool first,
                     const iop::StringView msg) const noexcept {
    if (first) {
      this->log(level, msg, iop::LogType::STARTEND, defaultLineTermination);
    } else {
      this->log(level, msg, iop::LogType::END, defaultLineTermination);
    }
  }

  ~Log() = default;
  Log(Log const &other) noexcept = default;
  Log(Log &&other) noexcept = default;
  auto operator=(Log const &other) noexcept -> Log & = default;
  auto operator=(Log &&other) noexcept -> Log & = default;

private:
  void printLogType(const iop::LogType &logType,
                    const iop::LogLevel &level) const noexcept;

  void log(const iop::LogLevel &level, const iop::StaticString &msg,
           const iop::LogType &logType,
           const iop::StaticString &lineTermination) const noexcept;
  void log(const iop::LogLevel &level, const iop::StringView &msg,
           const iop::LogType &logType,
           const iop::StaticString &lineTermination) const noexcept;

  static void print(const __FlashStringHelper *str) noexcept;
  static void print(const char *str) noexcept;
  static void reportLog() noexcept;

  auto levelToString() const noexcept -> iop::StaticString {
    switch (this->level()) {
    case iop::LogLevel::TRACE:
      return F("TRACE");
    case iop::LogLevel::DEBUG:
      return F("DEBUG");
    case iop::LogLevel::INFO:
      return F("INFO");
    case iop::LogLevel::WARN:
      return F("WARN");
    case iop::LogLevel::ERROR:
      return F("ERROR");
    case iop::LogLevel::CRIT:
      return F("CRIT");
    case iop::LogLevel::NO_LOG:
      return F("NO_LOG");
    }
    return F("UNKNOWN");
  }
};

#include "utils.hpp"
#ifndef IOP_SERIAL
#define IOP_LOG_DISABLED
#endif

#endif
