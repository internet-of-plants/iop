#ifndef IOP_LOG_H_
#define IOP_LOG_H_

#include <Arduino.h>

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
    LogLevel level;

  public:
    Log(const LogLevel level): level(level) {}
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
