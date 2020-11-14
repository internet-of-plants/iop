#ifndef IOP_LOG_H_
#define IOP_LOG_H_

#include <WString.h>
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

static const char * const PROGMEM defaultLineTerminationChar = "\n";
static const StaticString defaultLineTermination(FPSTR(defaultLineTerminationChar));

/// Logger with its own log level and target
class Log {
  private:
    LogLevel logLevel;
    StaticString targetLogger;
    bool flush = true;

    void printLogType(const enum LogType logType, const LogLevel level) const;

  void log(const LogLevel level, const StaticString msg, const enum LogType logType, const StaticString lineTermination) const;
  void log(const LogLevel level, const char * const msg, const enum LogType logType, const StaticString lineTermination) const;
  public:
    Log(const LogLevel level, const StaticString target):
      logLevel{level},
      targetLogger(target) {}
    Log(const LogLevel level, const StaticString target, const bool flush):
      logLevel{level},
      targetLogger(target),
      flush{flush} {}
    Log(Log& other) = delete;
    void operator=(Log& other) noexcept {
      this->logLevel = std::move(other.logLevel);
      this->targetLogger = std::move(other.targetLogger);
    }
    void operator=(Log&& other) noexcept {
      this->logLevel = std::move(other.logLevel);
      this->targetLogger = std::move(other.targetLogger);
    }
    LogLevel level() const { return this->logLevel; }
    StaticString target() const { return this->targetLogger; }
    void setup() const;

    void trace(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void debug(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void info(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void warn(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void error(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void crit(const StaticString & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;

    void trace(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void debug(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void info(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void warn(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void error(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
    void crit(const StringView & msg, const enum LogType logType = START, const StaticString lineTermination = defaultLineTermination) const;
};

#include <utils.hpp>
#ifndef IOP_SERIAL
  #define IOP_LOG_DISABLED
#endif

#endif
