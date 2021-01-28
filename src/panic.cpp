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

PROGMEM_STRING(missingHost, "No host available");
static const Api api(host.asRef().expect(missingHost), LogLevel::TRACE);

static const Flash flash(LogLevel::TRACE);

void upgrade() noexcept {
  IOP_TRACE();
  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.isNone())
    return;

  const auto &token = UNWRAP_REF(maybeToken);
  const auto mac = utils::macAddress();
  const auto status = api.upgrade(token, mac, utils::hashSketch());

  switch (status) {
  case ApiStatus::FORBIDDEN:
    logger.warn(F("Invalid auth token, but keeping since at panic_"));
    return;

  case ApiStatus::NOT_FOUND:
    // No updates available

  case ApiStatus::BROKEN_SERVER:
    // Central server is broken. Nothing we can do besides retrying later
    return;

  case ApiStatus::CLIENT_BUFFER_OVERFLOW:
    panic_(F("Api::upgrade internal buffer overflow"));

  case ApiStatus::BROKEN_PIPE:
  case ApiStatus::TIMEOUT:
  case ApiStatus::NO_CONNECTION:
    // Nothing to be done besides retrying later

  case ApiStatus::MUST_UPGRADE: // Bruh
  case ApiStatus::OK:           // Cool beans
    // Unreachable
    return;
  }

  const auto str = Network::apiStatusToString(status);
  logger.error(F("Bad status, EventLoop::handleInterrupt "), str);
}

// TODO(pc): save unique panics to flash
// TODO(pc): dump stackstrace on panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
auto reportPanic(const StringView msg, const StaticString file,
                 const uint32_t line, const StringView func) -> bool {
  IOP_TRACE();

  const auto maybeToken = flash.readAuthToken();
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

  static auto mac = utils::macAddress();
  const auto status = api.reportPanic(token, mac, panicData);
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

  case ApiStatus::NOT_FOUND:
  case ApiStatus::BROKEN_SERVER:
    // Nothing we can do besides waiting.
    logger.crit(F("Api::reportPanic is broken"));
    return false;

  case ApiStatus::BROKEN_PIPE:
  case ApiStatus::TIMEOUT:
  case ApiStatus::NO_CONNECTION:
    // Nothing to be done besides retrying later
    return false;

  case ApiStatus::OK:
    logger.info(F("Reported panic to server successfully"));
    return true;

  case ApiStatus::MUST_UPGRADE:
    // No need to upgrade since we handle this since we try to update later
    return true;
  }
  const auto str = Network::apiStatusToString(status);
  logger.error(F("Unexpected status, panic.h: reportPanic: "), str);
  return false;
}

void entry(const StringView msg, const StaticString file, const uint32_t line,
           const StringView func) {
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

void halt(StringView msg, StaticString file, uint32_t line, StringView func)
    __attribute__((noreturn));

void halt(const StringView msg, const StaticString file, const uint32_t line,
          const StringView func) {
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

void panic__(const StringView msg, const StaticString file, const uint32_t line,
             const StringView func) {
  IOP_TRACE();
  entry(msg, file, line, func);
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), msg);
  halt(msg, file, line, func);
}

void panic__(const StaticString msg, const StaticString file,
             const uint32_t line, const StringView func) {
  IOP_TRACE();
  entry(String(msg.get()), file, line, func);
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), msg);
  halt(String(msg.get()), file, line, func);
}

void panic__(const String &msg, const StaticString file, const uint32_t line,
             const StringView func) {
  IOP_TRACE();
  panic__(StringView(msg), file, line, func);
}

void panic__(const std::string &msg, const StaticString file,
             const uint32_t line, const StringView func) {
  IOP_TRACE();
  panic__(StringView(msg), file, line, func);
}

void panic__(const __FlashStringHelper *msg, const StaticString file,
             const uint32_t line, const StringView func) {
  IOP_TRACE();
  panic__(StaticString(msg), file, line, func);
}