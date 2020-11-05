#include <log.hpp>
#include <configuration.h>

void Log::setup() const {
  Serial.begin(9600);
  logger.info("Setup");
}

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

void Log::log(const enum LogLevel level, const String msg) const {
  #ifndef IOP_SERIAL
    return;
  #endif
  #ifdef IOP_SERIAL
  if (this->level > level) return;

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
    case CRIT:
      levelName = "CRIT";
  }
  Serial.println("[" + levelName + "]: " + msg);
  Serial.flush();
  #endif
}
