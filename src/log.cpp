#include "log.hpp"

#ifndef IOP_LOG_DISABLED
#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "unsafe_raw_string.hpp"
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
static Api api(host.asRef().expect(missingHost), LogLevel::WARN);
static Flash flash(LogLevel::WARN);
#ifdef IOP_NETWORK_LOGGING
static bool logNetwork = true;
#endif

void Log::print(const __FlashStringHelper *str) noexcept {
  Serial.print(str);
#ifdef IOP_NETWORK_LOGGING
  if (logNetwork) {
    currentLog += str;
    byteRate.addBytes(strlen_P(reinterpret_cast<PGM_P>(str)));
  }
#endif
}
void Log::print(const char *str) noexcept {
  Serial.print(str);
#ifdef IOP_NETWORK_LOGGING
  if (logNetwork) {
    currentLog += str;
    byteRate.addBytes(strlen(str));
  }
#endif
}

void Log::reportLog() noexcept {
#ifdef IOP_NETWORK_LOGGING
  if (!logNetwork)
    return;

  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.isSome()) {
    logNetwork = false;
    api.registerLog(UNWRAP_REF(maybeToken), utils::macAddress(), currentLog);
    logNetwork = true;
  }
  currentLog.clear();
#endif
}

void Log::setup() noexcept {
  constexpr const uint32_t BAUD_RATE = 115200;
  Serial.begin(BAUD_RATE);
  if (logLevel <= LogLevel::TRACE)
    Serial.setDebugOutput(true);

  constexpr const uint32_t thirtySec = 30 * 1000;
  const auto end = millis() + thirtySec;

  while (!Serial && millis() < end)
    yield();

  Log(LogLevel::INFO, F("LOG")).info(F("Setup"));
}

void Log::printLogType(const LogType logType,
                       const LogLevel level) const noexcept {
  if (logLevel_ == LogLevel::NO_LOG)
    return;

  switch (logType) {
  case LogType::CONTINUITY:
    break;

  case LogType::START:
    this->print(F("["));

    switch (level) {
    case LogLevel::TRACE:
      this->print(F("TRACE"));
      break;

    case LogLevel::DEBUG:
      this->print(F("DEBUG"));
      break;

    case LogLevel::INFO:
      this->print(F("INFO"));
      break;

    case LogLevel::WARN:
      this->print(F("WARN"));
      break;

    case LogLevel::ERROR:
      this->print(F("ERROR"));
      break;

    case LogLevel::NO_LOG:
      return;

    case LogLevel::CRIT:
    default:
      this->print(F("CRIT"));
      break;
    }

    this->print(F("] "));
    this->print(this->targetLogger.get());
    this->print(F(": "));
  };
}

void Log::log(const LogLevel level, const StaticString msg,
              const LogType logType,
              const StaticString lineTermination) const noexcept {
  if (this->logLevel_ > level)
    return;

  if (this->flush)
    Serial.flush();

  this->printLogType(logType, level);

  this->print(msg.get());
  this->print(lineTermination.get());
  if (this->flush)
    Serial.flush();
}

void Log::log(const LogLevel level, const StringView msg, const LogType logType,
              const StaticString lineTermination) const noexcept {
  if (this->logLevel_ > level)
    return;

  if (this->flush)
    Serial.flush();

  this->printLogType(logType, level);
  this->print(msg.get());
  this->print(lineTermination.get());
  if (this->flush)
    Serial.flush();
}
#else
void Log::setup() noexcept {}
void Log::printLogType(const LogType logType,
                       const LogLevel level) const noexcept {
  (void)logType;
  (void)level;
}
void Log::log(const LogLevel level, const StringView msg, const LogType logType,
              const StaticString lineTermination) const noexcept {
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
void Log::log(const LogLevel level, const StaticString msg,
              const LogType logType,
              const StaticString lineTermination) const noexcept {
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
#endif
