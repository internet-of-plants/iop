#include "panic.hpp"

#include "EEPROM.h"
#include "ESP8266WiFi.h"

#include "log.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "unsafe_raw_string.hpp"

#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"

PROGMEM_STRING(logTarget, "PANIC")
static const Log logger(LogLevel::TRACE, logTarget);
static bool panicking = false;

static const Api api(uri, LogLevel::TRACE);
static const Flash flash(LogLevel::TRACE);

void upgrade() noexcept {
  IOP_TRACE();
  const auto &maybeToken = flash.readAuthToken();
  if (maybeToken.isNone())
    return;

  const auto &token = UNWRAP_REF(maybeToken);
  const auto &mac = utils::macAddress();
  const auto status = api.upgrade(token, mac, utils::hashSketch());

  switch (status) {
  case ApiStatus::FORBIDDEN:
    logger.warn(F("Invalid auth token, but keeping since at panic_"));
    return;

  case ApiStatus::CLIENT_BUFFER_OVERFLOW:
    panic_(F("Api::upgrade internal buffer overflow"));

  // Already logged at the network level
  case ApiStatus::CONNECTION_ISSUES:
  case ApiStatus::BROKEN_SERVER:
    // Nothing to be done besides retrying later

  case ApiStatus::OK: // Cool beans, triggered if no updates are available too
    return;
  }

  const auto str = Network::apiStatusToString(status);
  logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
}

// TODO(pc): save unique panics to flash
// TODO(pc): dump stackstrace on panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
auto reportPanic(const StringView &msg, const StaticString &file,
                 const uint32_t line, const StringView &func) -> bool {
  IOP_TRACE();

  const auto &maybeToken = flash.readAuthToken();
  if (maybeToken.isNone()) {
    logger.crit(F("No auth token, unable to report panic"));
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
  // TODO(pc): We could broadcast panics to other devices in the same network
  // if Api::reportPanic fails

  switch (status) {
  case ApiStatus::FORBIDDEN:
    logger.warn(F("Invalid auth token, but keeping since at panic"));
    return false;

  case ApiStatus::CLIENT_BUFFER_OVERFLOW:
    // TODO(pc): deal with this, but how? Truncating the msg?
    // Should we have an endpoint to report this type of error that can't
    // trigger it?
    logger.crit(F("Api::reportPanic client buffer overflow"));
    return false;

  case ApiStatus::BROKEN_SERVER:
    // Nothing we can do besides waiting.
    logger.crit(F("Api::reportPanic is broken"));
    return false;

  case ApiStatus::CONNECTION_ISSUES:
    // Nothing to be done besides retrying later
    return false;

  case ApiStatus::OK:
    logger.info(F("Reported panic to server successfully"));
    return true;
  }
  const auto str = Network::apiStatusToString(status);
  logger.error(F("Unexpected status, panic.h: reportPanic: "), str);
  return false;
}

void entry(const StringView &msg, const StaticString &file, const uint32_t line,
           const StringView &func) {
  IOP_TRACE();
  if (panicking) {
    logger.crit(F("PANICK REENTRY: Line "), std::to_string(line),
                F(" of file "), file, F(" inside "), func, F(": "), msg);
    ESP.deepSleep(0);
    __panic_func(file.asCharPtr(), line, func.get());
  }
  panicking = true;

  constexpr const uint16_t oneSecond = 1000;
  delay(oneSecond);
}

void halt(const StringView &msg, const StaticString &file, uint32_t line,
          const StringView &func) __attribute__((noreturn));

void halt(const StringView &msg, const StaticString &file, const uint32_t line,
          const StringView &func) {
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

    if (Network::isConnected()) {
      if (!reportedPanic)
        reportedPanic = reportPanic(msg, file, line, func);

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
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(StringView msg, const StaticString &file, const uint32_t line,
             const StringView &func) {
  IOP_TRACE();
  entry(msg, file, line, func);
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), msg);
  halt(std::move(msg), file, line, func);
}

void panic__(StaticString msg, const StaticString &file, const uint32_t line,
             const StringView &func) {
  IOP_TRACE();
  String msg_(msg.get());
  entry(msg_, file, line, func);
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), std::move(msg));
  halt(msg_, file, line, func);
}

void panic__(const String &msg, const StaticString &file, const uint32_t line,
             const StringView &func) {
  IOP_TRACE();
  panic__(StringView(msg), file, line, func);
}

void panic__(const std::string &msg, const StaticString &file,
             const uint32_t &line, const StringView &func) {
  IOP_TRACE();
  panic__(StringView(msg), file, line, func);
}

void panic__(const __FlashStringHelper *msg, const StaticString &file,
             const uint32_t line, const StringView &func) {
  IOP_TRACE();
  panic__(StaticString(msg), file, line, func);
}