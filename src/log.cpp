#include "core/log.hpp"
#include "core/static_runner.hpp"

#include "utils.hpp" // Imports IOP_SERIAL if available
#ifdef IOP_SERIAL

static void staticPrinter(const __FlashStringHelper *str,
                          const iop::LogType kind) noexcept;
static void viewPrinter(const char *str, const iop::LogType kind) noexcept;
static void setuper(iop::LogLevel level) noexcept;
static void flusher() noexcept;

static iop::LogHook hook(viewPrinter, staticPrinter, setuper, flusher);

// Other static variables may be constructed before this. So their
// construction will use the default logger. Even if you disabled logging.
// You should not use globaol static variables, and if so lazily instantiate
// them
static auto hookSetter = iop::StaticRunner([] { iop::Log::setHook(hook); });

#ifdef IOP_NETWORK_LOGGING
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

static bool logNetwork = true;
void reportLog() noexcept {
  if (!logNetwork)
    return;

  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.isSome()) {
    logNetwork = false;
    api.registerLog(UNWRAP_REF(maybeToken), utils::macAddress(), currentLog);
    logNetwork = true;
  }
  currentLog.clear();
}
#endif

static void staticPrinter(const __FlashStringHelper *str,
                          const iop::LogType kind) noexcept {
  Serial.print(str);
#ifdef IOP_NETWORK_LOGGING
  if (logNetwork) {
    currentLog += str;
    byteRate.addBytes(strlen_P(reinterpret_cast<PGM_P>(str)));
  }
  if (kind == LogType::END || kind == LogType::STARTEND)
    this->reportLog();
#endif
}
static void viewPrinter(const char *str, const iop::LogType kind) noexcept {
  Serial.print(str);
#ifdef IOP_NETWORK_LOGGING
  if (logNetwork) {
    currentLog += str;
    byteRate.addBytes(strlen(str));
  }
  if (kind == iop::LogType::END || kind == iop::LogType::STARTEND)
    Log::reportLog();
#endif
}
static void flusher() noexcept { iop::LogHook::defaultFlusher(); }
static void setuper(iop::LogLevel level) noexcept {
  iop::LogHook::defaultSetuper(level);
}
#else
static void setuper(iop::LogLevel level) noexcept {
  Serial.end();
  (void)level;
}
static void flusher() noexcept {}
static void viewPrinter(const char *str, const iop::LogType kind) noexcept {}
static void staticPrinter(const __FlashStringHelper *str,
                          const iop::LogType kind) noexcept {}
#endif
