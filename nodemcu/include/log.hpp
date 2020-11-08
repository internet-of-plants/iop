#ifndef IOP_LOG_H_
#define IOP_LOG_H_

#include <Arduino.h>
#include <option.hpp>

enum LogLevel {
  TRACE = 0,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  CRIT
};

class Log {
  private:
    LogLevel logLevel;
    String targetLogger; 

  public:
    Log(const LogLevel level, const String target): logLevel{level}, targetLogger(target) {}
    Log(Log& other) = delete;
    void operator=(Log& other) = delete;
    void operator=(Log&& other) noexcept {
      this->logLevel = other.logLevel;
      this->targetLogger = other.targetLogger;
    }
    LogLevel level() const { return this->logLevel; }
    String target() const { return this->targetLogger; }
    void setup() const;
    void trace(const String msg) const;
    void debug(const String msg) const;
    void info(const String msg) const;
    void warn(const String msg) const;
    void error(const String msg) const;
    void crit(const String msg) const;
    void log(const LogLevel level, const String msg) const;
};

#endif
