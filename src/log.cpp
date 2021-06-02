#include "core/log.hpp"
#include "core/static_runner.hpp"
#include "configuration.hpp"
#include "utils.hpp" // Imports IOP_SERIAL if available

#ifdef IOP_NETWORK_LOGGING

#include "driver/time.hpp"

static void staticPrinter(const __FlashStringHelper *str,
                          iop::LogLevel level, 
                          iop::LogType kind) noexcept;
static void viewPrinter(const char *str, iop::LogLevel level, iop::LogType kind) noexcept;
static void setuper(iop::LogLevel level) noexcept;
static void flusher() noexcept;

static iop::LogHook hook(viewPrinter, staticPrinter, setuper, flusher);

// Other static variables may be constructed before this. So their
// construction will use the default logger. Even if you disabled logging.
// You should not use globaol static variables, and if so lazily instantiate
// them
static auto hookSetter = iop::StaticRunner([] { iop::Log::setHook(hook); });

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
static std::string currentLog;

// TODO(pc): allow gradually sending bytes wifiClient->write(...) instead of
// buffering the log before sending We can use the already in place system of
// variadic templates to avoid this buffer
// TODO(pc): use ByteRate to allow grouping messages before sending, or reuse
// the TCP connection to many

static Api api(uri(), iop::LogLevel::WARN);
static Flash flash(iop::LogLevel::WARN);

static bool logNetwork = true;
void reportLog() noexcept {
  if (!logNetwork || !currentLog.length())
    return;

  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.has_value()) {
    logNetwork = false;
    api.registerLog(iop::unwrap_ref(maybeToken, IOP_CTX()), currentLog);
    logNetwork = true;
  }
  currentLog.clear();
}

static void staticPrinter(const __FlashStringHelper *str,
                          const iop::LogLevel level,
                          const iop::LogType kind) noexcept {
  iop::LogHook::defaultStaticPrinter(str, level, kind);

  const auto charArray = reinterpret_cast<PGM_P>(str);
  if (logNetwork && level >= iop::LogLevel::CRIT) {
    currentLog += charArray;
    byteRate.addBytes(strlen_P(charArray));
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND)
      reportLog();
  }
}
static void viewPrinter(const char *str, const iop::LogLevel level, const iop::LogType kind) noexcept {
  iop::LogHook::defaultViewPrinter(str, level, kind);

  if (logNetwork && level >= iop::LogLevel::CRIT) {
    currentLog += str;
    byteRate.addBytes(strlen(str));
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND)
      reportLog();
  }
}
static void flusher() noexcept { iop::LogHook::defaultFlusher(); }
static void setuper(iop::LogLevel level) noexcept {
  iop::LogHook::defaultSetuper(level);
}
#endif
