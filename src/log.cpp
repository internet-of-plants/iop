#include "log.hpp"

#ifndef IOP_LOG_DISABLED
#include "api.hpp"
#include "flash.hpp"

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
static String currentLog;

// TODO(pc): allow gradually sending bytes wifiClient->write(...) instead of
// buffering the log before sending We can use the already in place system of
// variadic templates to avoid this buffer
// TODO(pc): use ByteRate to allow grouping messages before sending, or reuse
// the TCP connection to many

static Api api(uri, iop::LogLevel::WARN);
static Flash flash(iop::LogLevel::WARN);

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
  if (logLevel <= iop::LogLevel::DEBUG)
    Serial.setDebugOutput(true);

  constexpr const uint32_t thirtySec = 30 * 1000;
  const auto end = millis() + thirtySec;

  while (!Serial && millis() < end)
    yield();
}

void Log::printLogType(const iop::LogType &logType,
                       const iop::LogLevel &level) const noexcept {
  if (logLevel_ == iop::LogLevel::NO_LOG)
    return;

  switch (logType) {
  case iop::LogType::CONTINUITY:
  case iop::LogType::END:
    break;

  case iop::LogType::START:
  case iop::LogType::STARTEND:
    this->print(F("["));
    this->print(this->levelToString().get());
    this->print(F("] "));
    this->print(this->targetLogger.get());
    this->print(F(": "));
  };
}

void Log::log(const iop::LogLevel &level, const iop::StaticString &msg,
              const iop::LogType &logType,
              const iop::StaticString &lineTermination) const noexcept {
  if (this->logLevel_ > level)
    return;

  Serial.flush();
  this->printLogType(logType, level);
  this->print(msg.get());
  this->print(lineTermination.get());
  Serial.flush();

  if (logType == iop::LogType::END || logType == iop::LogType::STARTEND)
    this->reportLog();
}

void Log::log(const iop::LogLevel &level, const iop::StringView &msg,
              const iop::LogType &logType,
              const iop::StaticString &lineTermination) const noexcept {
  if (this->logLevel_ > level)
    return;

  Serial.flush();
  this->printLogType(logType, level);
  this->print(msg.get());
  this->print(lineTermination.get());
  Serial.flush();

  if (logType == iop::LogType::END || logType == iop::LogType::STARTEND)
    this->reportLog();
}
#else
void Log::setup() noexcept {}
void Log::reportLog() noexcept {}
void Log::printiop::LogType(
    const iop::LogType &logType,
    const iop::LogLevel &reportiop::LogLevel) const noexcept {
  (void)*this;
  (void)logType;
  (void)reportiop::LogLevel;
}
void Log::log(const iop::LogLevel &level, const iop::StringView &msg,
              const iop::LogType &logType,
              const iop::StaticString &lineTermination) const noexcept {
  (void)*this;
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
void Log::log(const iop::LogLevel &level, const iop::StaticString &msg,
              const iop::LogType &logType,
              const iop::StaticString &lineTermination) const noexcept {
  (void)*this;
  (void)level;
  (void)msg;
  (void)logType;
  (void)lineTermination;
}
#endif
