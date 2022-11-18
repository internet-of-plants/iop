#include "iop-hal/panic.hpp"
#include "iop-hal/device.hpp"
#include "iop/loop.hpp"
#include "iop/api.hpp"
#include "iop-hal/log.hpp"

namespace iop {
auto update() noexcept -> void {
  IOP_TRACE();
  const auto token = iop::eventLoop.storage().token();
  if (!token)
    return;
  const auto status = iop::eventLoop.api().update(*token);

  switch (status) {
  case iop_hal::UpdateStatus::UNAUTHORIZED:
    // TODO: think about allowing global updates (if you are logged out and panicking get a safe global version and recover from it)
    // This brings security problems of getting your app hijacked and complicates binary signing
    iop::panicLogger().warnln(IOP_STR("Invalid auth token, but keeping since at iop_panic"));
    return;

  case iop_hal::UpdateStatus::BROKEN_CLIENT:
    iop_panic(IOP_STR("Api::update internal buffer overflow"));

  // Already logged at the network level
  case iop_hal::UpdateStatus::IO_ERROR:
  case iop_hal::UpdateStatus::BROKEN_SERVER:
    // Nothing to be done besides retrying later

  case iop_hal::UpdateStatus::NO_UPGRADE: // Cool beans
    return;
  }

  iop::panicLogger().errorln(IOP_STR("Bad status, panic::update"));
}

// TODO(pc): dump stackstrace on iop_panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
auto reportPanic(const std::string_view &msg, const iop::StaticString &file, const uint32_t line, const iop::StaticString &func) noexcept -> bool {
  IOP_TRACE();

  // Prevents network logging
  iop::Log::takeHook();

  const auto token = iop::eventLoop.storage().token();
  if (!token) {
    iop::panicLogger().critln(IOP_STR("No auth token, unable to report iop_panic"));
    return false;
  }

  const auto panicData = iop::PanicData(msg, file, line, func);
  const auto status = iop::eventLoop.api().reportPanic(*token, panicData);

  switch (status) {
  case iop::NetworkStatus::UNAUTHORIZED:
    iop::panicLogger().warnln(IOP_STR("Invalid auth token, but keeping since at iop_panic"));
    return false;

  case iop::NetworkStatus::BROKEN_CLIENT:
    iop::panicLogger().critln(IOP_STR("Unreachable as Api::reportPanic truncates the msg until it works"));
    return false;

  case iop::NetworkStatus::BROKEN_SERVER:
    iop::panicLogger().critln(IOP_STR("Api::reportPanic is broken, will retry again later"));
    return false;

  case iop::NetworkStatus::IO_ERROR:
    iop::panicLogger().critln(IOP_STR("IO Error: will retry again later"));
    return false;

  case iop::NetworkStatus::OK:
    iop::panicLogger().infoln(IOP_STR("Reported iop_panic to server successfully"));
    return true;
  }
  iop::panicLogger().errorln(IOP_STR("Unexpected status Api::reportPanic"));
  return false;
}

static void halt(const std::string_view &msg, iop::CodePoint const &point) noexcept __attribute__((noreturn));

static void halt(const std::string_view &msg, iop::CodePoint const &point) noexcept {
  IOP_TRACE();

  constexpr const uint32_t oneHour = ((uint32_t)60) * 60;

  auto reportedPanic = false;
  while (true) {
    if (!iop::eventLoop.storage().wifi()) {
      iop::panicLogger().warnln(IOP_STR("Nothing we can do, no wifi config available"));
      break;
    }

    if (!iop::eventLoop.storage().token()) {
      iop::panicLogger().warnln(IOP_STR("Nothing we can do, no auth token available"));
      break;
    }

    if (iop::Network::isConnected()) {
      if (!reportedPanic)
        reportedPanic = reportPanic(msg, point.file(), point.line(), point.func());

      // Panic data is lost if report fails but update works
      // Doesn't return if update succeeds
      update();
    } else {
      iop::panicLogger().warnln(IOP_STR("No network, unable to recover"));
    }
    iop_hal::device.deepSleep(oneHour);
  }

  iop_hal::thisThread.halt();
}

iop::PanicHook hook(iop::PanicHook::defaultViewPanic, iop::PanicHook::defaultStaticPanic, iop::PanicHook::defaultEntry, halt, iop::PanicHook::defaultCleanup);

namespace panic {
auto setup() noexcept -> void { iop::setPanicHook(hook); }
auto setCleanup(iop::PanicHook::Cleanup cleanup) noexcept -> void {
  hook.cleanup = cleanup;
  iop::setPanicHook(hook);
}
}
}