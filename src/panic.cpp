#include "core/panic.hpp"
#include "configuration.hpp"

#include "loop.hpp"
#include "driver/device.hpp"

void upgrade() noexcept {
  IOP_TRACE();
  const auto &maybeToken = unused4KbSysStack.loop().flash().readAuthToken();
  if (!maybeToken.has_value())
    return;

  const auto &token = iop::unwrap_ref(maybeToken, IOP_CTX());
  const auto status = unused4KbSysStack.loop().api().upgrade(token);

  switch (status) {
  case iop::NetworkStatus::FORBIDDEN:
    // TODO: think about allowing global updates (if you are logged out and panicking get a safe global version and recover from it)
    // This brings security problems of getting your app hijacked and complicates binary signing
    iop::panicLogger().warn(F("Invalid auth token, but keeping since at iop_panic"));
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
  iop::panicLogger().error(F("Bad status, EventLoop::handleInterrupt "), str);
}

// TODO(pc): save unique panics to flash
// TODO(pc): dump stackstrace on iop_panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
auto reportPanic(const std::string_view &msg, const iop::StaticString &file,
                 const uint32_t line, const iop::StaticString &func) noexcept
    -> bool {
  IOP_TRACE();

  const auto &maybeToken = unused4KbSysStack.loop().flash().readAuthToken();
  if (!maybeToken.has_value()) {
    iop::panicLogger().crit(F("No auth token, unable to report iop_panic"));
    return false;
  }

  const auto &token = iop::unwrap_ref(maybeToken, IOP_CTX());
  const auto panicData = (PanicData){
      msg,
      file,
      line,
      func,
  };

  const auto status = unused4KbSysStack.loop().api().reportPanic(token, panicData);

  switch (status) {
  case iop::NetworkStatus::FORBIDDEN:
    iop::panicLogger().warn(F("Invalid auth token, but keeping since at iop_panic"));
    return false;

  case iop::NetworkStatus::CLIENT_BUFFER_OVERFLOW:
    // TODO(pc): deal with this, but how? Truncating the msg?
    // Should we have an endpoint to report this type of error that can't
    // trigger it?
    iop::panicLogger().crit(F("Api::reportPanic client buffer overflow"));
    return false;

  case iop::NetworkStatus::BROKEN_SERVER:
    // Nothing we can do besides waiting.
    iop::panicLogger().crit(F("Api::reportPanic is broken"));
    return false;

  case iop::NetworkStatus::CONNECTION_ISSUES:
    // Nothing to be done besides retrying later
    return false;

  case iop::NetworkStatus::OK:
    iop::panicLogger().info(F("Reported iop_panic to server successfully"));
    return true;
  }
  const auto str = iop::Network::apiStatusToString(status);
  iop::panicLogger().error(F("Unexpected status, iop_panic.h: reportPanic: "), str);
  return false;
}

static void halt(const std::string_view &msg,
                 iop::CodePoint const &point) noexcept
    __attribute__((noreturn));

static void halt(const std::string_view &msg,
                 iop::CodePoint const &point) noexcept {
  IOP_TRACE();
  auto reportedPanic = false;

  constexpr const uint32_t oneHour = ((uint32_t)60) * 60;
  while (true) {
    if (!unused4KbSysStack.loop().flash().readWifiConfig().has_value()) {
      iop::panicLogger().warn(F("Nothing we can do, no wifi config available"));
      break;
    }

    if (!unused4KbSysStack.loop().flash().readAuthToken().has_value()) {
      iop::panicLogger().warn(F("Nothing we can do, no auth token available"));
      break;
    }

    if (WiFi.getMode() == WIFI_OFF) {
      iop::panicLogger().crit(F("WiFi is disabled, unable to recover"));
      break;
    }

    if (iop::Network::isConnected()) {
      if (!reportedPanic)
        reportedPanic =
            reportPanic(msg, point.file(), point.line(), point.func());

      // Panic data is lost if report fails but upgrade works
      // Doesn't return if upgrade succeeds
      upgrade();
    } else {
      iop::panicLogger().warn(F("No network, unable to recover"));
    }
    driver::device.deepSleep(oneHour);

    // Let's allow the wifi to reconnect
    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    WiFi.reconnect();
    WiFi.waitForConnectResult();
  }

  driver::device.deepSleep(0);
  driver::thisThread.panic_();
}

iop::PanicHook hook(iop::PanicHook::defaultViewPanic,
                           iop::PanicHook::defaultStaticPanic,
                           iop::PanicHook::defaultEntry, halt);

namespace panic {
void setup() noexcept {
  iop::setPanicHook(hook);
}
}