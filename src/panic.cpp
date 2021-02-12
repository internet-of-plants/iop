#include "core/panic.hpp"
#include "api.hpp"
#include "configuration.hpp"
#include "core/static_runner.hpp"
#include "flash.hpp"

PROGMEM_STRING(logTarget, "PANIC")
static const iop::Log logger(iop::LogLevel::TRACE, logTarget);

static const Api api(uri, iop::LogLevel::TRACE);
static const Flash flash(iop::LogLevel::TRACE);

void upgrade() noexcept {
  IOP_TRACE();
  const auto &maybeToken = flash.readAuthToken();
  if (maybeToken.isNone())
    return;

  const auto &token = UNWRAP_REF(maybeToken);
  const auto &mac = iop::macAddress();
  const auto status = api.upgrade(token, mac, iop::hashSketch());

  switch (status) {
  case iop::NetworkStatus::FORBIDDEN:
    logger.warn(F("Invalid auth token, but keeping since at iop_panic"));
    return;

  case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    iop_panic(F("Api::upgrade internal buffer overflow"));

  // Already logged at the network level
  case iop::NetworkStatus::CONNECTION_ISSUES:
  case iop::NetworkStatus::BROKEN_SERVER:
    // Nothing to be done besides retrying later

  case iop::NetworkStatus::OK: // Cool beans, triggered if no updates are
                               // available too
    return;
  }

  const auto str = iop::Network::apiStatusToString(status);
  logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
}

// TODO(pc): save unique panics to flash
// TODO(pc): dump stackstrace on iop_panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
auto reportPanic(const iop::StringView &msg, const iop::StaticString &file,
                 const uint32_t line, const iop::StringView &func) noexcept
    -> bool {
  IOP_TRACE();

  const auto &maybeToken = flash.readAuthToken();
  if (maybeToken.isNone()) {
    logger.crit(F("No auth token, unable to report iop_panic"));
    return false;
  }

  const auto &token = UNWRAP_REF(maybeToken);
  const auto panicData = (PanicData){
      msg,
      file,
      line,
      func,
  };

  const auto status = api.reportPanic(token, panicData);

  switch (status) {
  case iop::NetworkStatus::FORBIDDEN:
    logger.warn(F("Invalid auth token, but keeping since at iop_panic"));
    return false;

  case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    // TODO(pc): deal with this, but how? Truncating the msg?
    // Should we have an endpoint to report this type of error that can't
    // trigger it?
    logger.crit(F("Api::reportPanic client buffer overflow"));
    return false;

  case iop::NetworkStatus::BROKEN_SERVER:
    // Nothing we can do besides waiting.
    logger.crit(F("Api::reportPanic is broken"));
    return false;

  case iop::NetworkStatus::CONNECTION_ISSUES:
    // Nothing to be done besides retrying later
    return false;

  case iop::NetworkStatus::OK:
    logger.info(F("Reported iop_panic to server successfully"));
    return true;
  }
  const auto str = iop::Network::apiStatusToString(status);
  logger.error(F("Unexpected status, iop_panic.h: reportPanic: "), str);
  return false;
}

static void halt(const iop::StringView &msg,
                 iop::CodePoint const &point) noexcept
    __attribute__((noreturn));

static void halt(const iop::StringView &msg,
                 iop::CodePoint const &point) noexcept {
  IOP_TRACE();
  auto reportedPanic = false;

  constexpr const uint32_t tenMinutesUs = 10 * 60 * 1000;
  constexpr const uint32_t oneHourUs = 60 * 60 * 1000;
  while (true) {
    if (flash.readWifiConfig().isNone()) {
      logger.warn(F("Nothing we can do, no wifi config available"));
      break;
    }

    if (flash.readAuthToken().isNone()) {
      logger.warn(F("Nothing we can do, no auth token available"));
      break;
    }

    if (WiFi.getMode() == WIFI_OFF) {
      logger.crit(F("WiFi is disabled, unable to recover"));
      break;
    }

    if (iop::Network::isConnected()) {
      if (!reportedPanic)
        reportedPanic =
            reportPanic(msg, point.file(), point.line(), point.func());

      // Panic data is lost if report fails but upgrade works
      // Doesn't return if upgrade succeeds
      upgrade();

      ESP.deepSleep(tenMinutesUs);
    } else {
      logger.warn(F("No network, unable to recover"));
      ESP.deepSleep(oneHourUs);
    }

    // Let's allow the wifi to reconnect
    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    WiFi.reconnect();
    WiFi.waitForConnectResult();
  }

  ESP.deepSleep(0);
  __panic_func(point.file().asCharPtr(), point.line(), point.func().get());
}

// Other static variables may be constructed before this. So their
// construction will use the default panic hook. You should not use global
// static variables, and if so lazily instantiate them
static iop::PanicHook hook(iop::PanicHook::defaultViewPanic,
                           iop::PanicHook::defaultStaticPanic,
                           iop::PanicHook::defaultEntry, halt);
static auto hookSetter = iop::StaticRunner([] { iop::setPanicHook(hook); });