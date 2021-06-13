#include "core/log.hpp"
#include "configuration.hpp"
#include "utils.hpp" // Imports IOP_SERIAL if available
#include "core/lazy.hpp"

#ifdef IOP_NETWORK_LOGGING

#include "driver/thread.hpp"

static void staticPrinter(const iop::StaticString str,
                          iop::LogLevel level, 
                          iop::LogType kind) noexcept;
static void viewPrinter(const std::string_view, iop::LogLevel level, iop::LogType kind) noexcept;
static void setuper(iop::LogLevel level) noexcept;
static void flusher() noexcept;

static iop::LogHook hook(viewPrinter, staticPrinter, setuper, flusher);

namespace network_logger {
  void setup() noexcept {
    iop::Log::setHook(hook);
    iop::Log::setup(logLevel);
  }
}

#include "api.hpp"
#include "flash.hpp"

class ByteRate {
  size_t lastBytesPerMinute{0};
  size_t bytes{0};
  uint64_t nextReset{0};

public:
  ByteRate() noexcept = default;

  void resetIfNeeded() noexcept {
    constexpr const size_t minutes = 5;
    constexpr const uint32_t fiveMin = minutes * 1000 * 60;
    this->nextReset = driver::thisThread.now() + fiveMin;
    this->lastBytesPerMinute = this->bytes / minutes;
    this->bytes = 0;
  }

  void addBytes(size_t bytes) noexcept {
    this->resetIfNeeded();
    this->bytes += bytes;
  }

  auto bytesPerMinute() const noexcept -> size_t {
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

static iop::Lazy<Api> api([]() { return Api(uri(), iop::LogLevel::WARN); });
static iop::Lazy<Flash> flash([]() { return Flash(iop::LogLevel::WARN); });

static bool logNetwork = true;
void reportLog() noexcept {
  if (!logNetwork || !currentLog.length())
    return;

  const auto maybeToken = flash->readAuthToken();
  if (maybeToken.has_value()) {
    logNetwork = false;
    api->registerLog(iop::unwrap_ref(maybeToken, IOP_CTX()), currentLog);
    logNetwork = true;
  }
  currentLog.clear();
}

static void staticPrinter(const iop::StaticString str,
                          const iop::LogLevel level,
                          const iop::LogType kind) noexcept {
  iop::LogHook::defaultStaticPrinter(str, level, kind);

  const auto charArray = str.asCharPtr();
  if (logNetwork && level >= iop::LogLevel::CRIT) {
    currentLog += charArray;
    byteRate.addBytes(strlen_P(charArray));
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND)
      reportLog();
  }
}
static void viewPrinter(const std::string_view str, const iop::LogLevel level, const iop::LogType kind) noexcept {
  iop::LogHook::defaultViewPrinter(str, level, kind);

  if (logNetwork && level >= iop::LogLevel::CRIT) {
    currentLog += str;
    byteRate.addBytes(str.length());
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND)
      reportLog();
  }
}
static void flusher() noexcept { iop::LogHook::defaultFlusher(); }
static void setuper(iop::LogLevel level) noexcept {
  iop::LogHook::defaultSetuper(level);
}
#else
namespace network_logger {
  void setup() noexcept {
    iop::Log::setup(logLevel);
  }
}
#endif
