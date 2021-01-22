#include "log.hpp"

#ifndef IOP_LOG_DISABLED
#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"
#include <Arduino.h>

class ByteRate {
  uint32_t lastBytesPerMinute{0};
  uint32_t bytes{0};
  uint64_t nextReset{0};

public:
  ByteRate() noexcept = default;

  void resetIfNeeded() noexcept {
    constexpr const uint8_t minutes = 5;
    constexpr const uint32_t fiveMin = minutes * 1000 * 60;
    this->nextReset = millis() + fiveMin;
    this->lastBytesPerMinute = this->bytes / minutes;
    this->bytes = 0;
  }

  void addBytes(uint16_t bytes) noexcept {
    this->resetIfNeeded();
    this->bytes += bytes;
  }

  auto bytesPerMinute() const noexcept -> uint32_t {
    return this->lastBytesPerMinute;
  }
};

static ByteRate byteRate;
static String currentLog; // NOLINT cert-err58-cpp

// TODO(pc): allow gradually sending bytes wifiClient->write(...) instead of
// buffering the log before sending We can use the already in place system of
// variadic templates to avoid this buffer
// TODO(pc): use ByteRate to allow grouping messages before sending, or reuse
// the TCP connection to many

PROGMEM_STRING(missingHost, "No host available");
static Api api(host.asRef().expect(missingHost), WARN);
static Flash flash(WARN);
static bool logNetwork = true;

void Log::print(const StaticString str) noexcept {
  Serial.print(str.get());
  if (logNetwork) {
    currentLog += str.get();
    byteRate.addBytes(str.length());
  }
}
void Log::print(const StringView str) noexcept {
  Serial.print(str.get());
  if (logNetwork) {
    currentLog += str.get();
    byteRate.addBytes(str.length());
  }
}

void Log::reportLog() noexcept {
  if (!logNetwork) {
    currentLog.clear();
    return;
  }

  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.isSome()) {
    const auto oldLogNetwork = logNetwork;
    logNetwork = false;

    static auto mac = utils::macAddress();
    api.registerLog(UNWRAP_REF(maybeToken), mac, currentLog);

    logNetwork = oldLogNetwork;
  }
  currentLog.clear();
}

void Log::setup() const noexcept {
  constexpr const uint32_t BAUD_RATE = 9600;
  Serial.begin(BAUD_RATE);

  constexpr const uint32_t thirtySec = 30 * 1000;
  const auto end = millis() + thirtySec;

  while (!Serial && millis() < end)
    yield();

  this->info(F("Setup"));
}

void Log::printLogType(const enum LogType logType,
                       const LogLevel level) const noexcept {
  switch (logType) {
  case CONTINUITY:
    break;
  case START:
    this->print(F("["));
    switch (level) {
    case TRACE:
      this->print(F("TRACE"));
      break;
    case DEBUG:
      this->print(F("DEBUG"));
      break;
    case INFO:
      this->print(F("INFO"));
      break;
    case WARN:
      this->print(F("WARN"));
      break;
    case ERROR:
      this->print(F("ERROR"));
      break;
    case NO_LOG:
      return;
    case CRIT:
    default:
      this->print(F("CRIT"));
      break;
    }
    this->print(F("] "));
    this->print(this->targetLogger);
    this->print(F(": "));
  };
}

void Log::log(const enum LogLevel level, const StaticString msg,
              const enum LogType logType,
              const StaticString lineTermination) const noexcept {
  if (this->logLevel > level)
    return;

  if (this->flush)
    Serial.flush();

  this->printLogType(logType, level);

  this->print(msg);
  this->print(lineTermination);
  if (this->flush)
    Serial.flush();
}

void Log::log(const enum LogLevel level, const StringView msg,
              const enum LogType logType,
              const StaticString lineTermination) const noexcept {
  if (this->logLevel > level)
    return;

  if (this->flush)
    Serial.flush();

  this->printLogType(logType, level);
  this->print(msg);
  this->print(lineTermination);
  if (this->flush)
    Serial.flush();
}
#else
void Log::setup() const {}
void Log::printLogType(const enum LogType logType,
                       const LogLevel level) const noexcept {
  (void)logType;
  (void)level;
}
void Log::log(const enum LogLevel level, const StringView msg,
              const enum LogType logType,
              const StaticString lineTermination) const noexcept {
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
void Log::log(const enum LogLevel level, const StaticString msg,
              const enum LogType logType,
              const StaticString lineTermination) const noexcept {
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
#endif
