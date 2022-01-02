#include "driver/log.hpp"
#include "configuration.hpp"
#include "loop.hpp"

#ifdef IOP_NETWORK_LOGGING

#include "driver/thread.hpp"

static void staticPrinter(const iop::StaticString str, iop::LogLevel level, iop::LogType kind) noexcept;
static void viewPrinter(const iop::StringView, iop::LogLevel level, iop::LogType kind) noexcept;
static void setuper(iop::LogLevel level) noexcept;
static void flusher() noexcept;

static iop::LogHook hook(viewPrinter, staticPrinter, setuper, flusher);

namespace network_logger {
  void setup() noexcept {
    iop::Log::setHook(hook);
    iop::Log::setup(config::logLevel);
  }
}

class ByteRate {
  uint64_t nextReset{0};
  size_t lastBytesPerMinute{0};
  size_t bytes{0};

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

static bool logNetwork = true;
void reportLog() noexcept {
  if (!logNetwork || !currentLog.length())
    return;

  driver::thisThread.yield();
  logNetwork = false;
  const auto token = eventLoop.flash().token();
  if (token) {
    eventLoop.api().registerLog(*token, currentLog);
  } else {
    iop::Log(iop::LogLevel::WARN, IOP_STATIC_STRING("NETWORK LOGGING")).warn(IOP_STATIC_STRING("Unable to log to the monitor server, not authenticated"));
  }
  logNetwork = true;
  currentLog.clear();
}

static void staticPrinter(const iop::StaticString str,
                          const iop::LogLevel level,
                          const iop::LogType kind) noexcept {
  iop::LogHook::defaultStaticPrinter(str, level, kind);

  if (logNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str.asCharPtr();
    byteRate.addBytes(str.length());
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      iop::LogHook::defaultStaticPrinter(IOP_STATIC_STRING("Logging to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
      reportLog();
      iop::LogHook::defaultStaticPrinter(IOP_STATIC_STRING("Logged to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}
static void viewPrinter(const iop::StringView str, const iop::LogLevel level, const iop::LogType kind) noexcept {
  iop::LogHook::defaultViewPrinter(str, level, kind);

  if (logNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str;
    byteRate.addBytes(str.length());
    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      iop::LogHook::defaultStaticPrinter(IOP_STATIC_STRING("Logging to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
      reportLog();
      iop::LogHook::defaultStaticPrinter(IOP_STATIC_STRING("Logged to network\n"), iop::LogLevel::INFO, iop::LogType::STARTEND);
    }
  }
}
static void flusher() noexcept { iop::LogHook::defaultFlusher(); }
static void setuper(iop::LogLevel level) noexcept {
  iop::LogHook::defaultSetuper(level);
}
#else
namespace network_logger {
  void setup() noexcept {
    iop::Log::setup(config::logLevel);
  }
}
#endif
