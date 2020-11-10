#include <log.hpp>

void Log::trace(const String msg) const {
  log(TRACE, msg);
}

void Log::debug(const String msg) const {
  log(DEBUG, msg);
}

void Log::info(const String msg) const {
  log(INFO, msg);
}

void Log::warn(const String msg) const {
  log(WARN, msg);
}

void Log::error(const String msg) const {
  log(ERROR, msg);
}

void Log::crit(const String msg) const {
  log(CRIT, msg);
}

#ifndef IOP_LOG_DISABLED
#include <Arduino.h>

void Log::setup() const {
  Serial.begin(9600);
  this->info("Setup");
}

void Log::log(const enum LogLevel level, const String msg) const {
  if (this->logLevel > level) return;

  String levelName;
  switch (level) {
    case TRACE:
      levelName = "TRACE";
      break;
    case DEBUG:
      levelName = "DEBUG";
      break;
    case INFO:
      levelName = "INFO";
      break;
    case WARN:
      levelName = "WARN";
      break;
    case ERROR:
      levelName = "ERROR";
      break;
    case CRIT:
      levelName = "CRIT";
      break;
  }
  Serial.flush();
  Serial.println("[" + levelName + "] " + this->targetLogger + ": " + msg);
  Serial.flush();
}
#endif

#ifdef IOP_LOG_DISABLED
void Log::setup() const {}
void Log::log(const enum LogLevel level, const String msg) const {
  (void) level;
  (void) msg;
}
#endif