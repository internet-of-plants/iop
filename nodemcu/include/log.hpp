#ifndef IOP_LOG_H_
#define IOP_LOG_H_

#include <option.hpp>
#include <static_string.hpp>
#include <string_view.hpp>

enum LogLevel {
  TRACE = 0,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  CRIT
};

enum LogType {
  CONTINUITY,
  START
};

PROGMEM_STRING(defaultLineTermination, "\n");

/// Logger with its own log level and target
class Log {
  private:
    LogLevel logLevel;
    StaticString targetLogger;
    bool flush = true;

    void printLogType(const enum LogType logType, const LogLevel level) const noexcept;

    void log(const LogLevel level, const StaticString msg, const enum LogType logType, const StaticString lineTermination) const noexcept;
    void log(const LogLevel level, const char * const msg, const enum LogType logType, const StaticString lineTermination) const noexcept;
  public:
    Log(const LogLevel level, const StaticString target) noexcept:
      logLevel{level},
      targetLogger(target) {}
    Log(const LogLevel level, const StaticString target, const bool flush) noexcept:
      logLevel{level},
      targetLogger(target),
      flush{flush} {}
    Log(Log& other) = delete;
    Log(Log&& other) = delete;
    Log& operator=(Log& other) = delete;
    Log& operator=(Log&& other) = delete;
    LogLevel level() const noexcept { return this->logLevel; }
    StaticString target() const noexcept { return this->targetLogger; }
    void setup() const noexcept;

    void trace(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void debug(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void info(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void warn(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void error(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void crit(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;

    void trace(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void debug(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void info(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void warn(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void error(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
    void crit(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const noexcept;
};

#include <utils.hpp>
#ifndef IOP_SERIAL
  #define IOP_LOG_DISABLED
#endif

#endif
