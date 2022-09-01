#include "iop-hal/log.hpp"
#include "iop/loop.hpp"

#include "iop-hal/thread.hpp"

static auto staticPrinter(const iop::StaticString str, iop::LogLevel level, iop::LogType kind) noexcept -> void;
static auto viewPrinter(const std::string_view, iop::LogLevel level, iop::LogType kind) noexcept -> void;
static auto setuper(iop::LogLevel level) noexcept -> void;
static auto flusher() noexcept -> void;

static auto hook = iop::LogHook(viewPrinter, staticPrinter, setuper, flusher);

namespace iop {
namespace network_logger {
  void setup() noexcept {
    iop::Log::setHook(hook);
    iop::Log::setup(IOP_LOG_LEVEL);
  }
}
}

static auto currentLog = std::string();
static auto logToNetwork = true;

void reportLog() noexcept {
  {
    const auto memory = iop_hal::thisThread.availableMemory();
    iop::LogHook::defaultStaticPrinter(IOP_STR("[DEBUG] Logger: Free Stack"), iop::LogLevel::DEBUG, iop::LogType::START);
    iop::LogHook::defaultViewPrinter(std::to_string(memory.availableStack), iop::LogLevel::DEBUG, iop::LogType::CONTINUITY);
    iop::LogHook::defaultStaticPrinter(IOP_STR("\n"), iop::LogLevel::DEBUG, iop::LogType::END);
  }
  if (!logToNetwork || !currentLog.length() || iop::wifi.status() != iop_hal::StationStatus::GOT_IP)
    return;
  logToNetwork = false;

  iop_hal::thisThread.yield();

  const auto token = iop::eventLoop.storage().token();
  if (token) {
    iop::eventLoop.api().registerLog(*token, currentLog);
  } else {
    iop::Log(iop::LogLevel::WARN, IOP_STR("NETWORK LOGGING")).warn(IOP_STR("Unable to log to the monitor server, not authenticated"));
  }
  logToNetwork = true;
  currentLog.clear();
}

static void staticPrinter(const iop::StaticString str, const iop::LogLevel level, const iop::LogType kind) noexcept {
  iop::LogHook::defaultStaticPrinter(str, level, kind);

  if (logToNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str.toString();

    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      if (level <= iop::LogLevel::DEBUG) iop::LogHook::defaultStaticPrinter(IOP_STR("[DEBUG] Logger: Logging to network\n"), iop::LogLevel::DEBUG, iop::LogType::STARTEND);
      reportLog();
      if (level <= iop::LogLevel::DEBUG) iop::LogHook::defaultStaticPrinter(IOP_STR("[DEBUG] Logger: Logged to network\n"), iop::LogLevel::DEBUG, iop::LogType::STARTEND);
    }
  }
}
static auto viewPrinter(const std::string_view str, const iop::LogLevel level, const iop::LogType kind) noexcept -> void {
  iop::LogHook::defaultViewPrinter(str, level, kind);

  if (logToNetwork && level >= iop::LogLevel::INFO) {
    currentLog += str;

    if (kind == iop::LogType::END || kind == iop::LogType::STARTEND) {
      if (level <= iop::LogLevel::DEBUG) iop::LogHook::defaultStaticPrinter(IOP_STR("[DEBUG] Logger: Logging to network\n"), iop::LogLevel::DEBUG, iop::LogType::STARTEND);
      reportLog();
      if (level <= iop::LogLevel::DEBUG) iop::LogHook::defaultStaticPrinter(IOP_STR("[DEBUG] Logger: Logged to network\n"), iop::LogLevel::DEBUG, iop::LogType::STARTEND);
    }
  }
}
static auto flusher() noexcept -> void { iop::LogHook::defaultFlusher(); }
static auto setuper(iop::LogLevel level) noexcept -> void { iop::LogHook::defaultSetuper(level); }
