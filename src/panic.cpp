#include "EEPROM.h"
#include "ESP8266WiFi.h"

#include "log.hpp"
#include "panic.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "unsafe_raw_string.hpp"

#include "api.hpp"
#include "configuration.h"
#include "flash.hpp"

// TODO(pc): save unique panics to flash
// TODO(pc): dump stackstrace on panic
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
void reportPanic(const StringView msg, const StaticString file,
                 const uint32_t line, const StringView func) {
  while (true) {
    const auto &host_ = UNWRAP_REF(host);
    const Log logger(CRIT, F("PANIC"));
    const Api api(host_, TRACE);
    if (!Api::isConnected()) {
      logger.crit(F("No connection, unable to report panic"));
      return;
    }

    const Flash flash(TRACE);
    const auto maybeToken = flash.readAuthToken();
    if (maybeToken.isNone()) {
      logger.crit(F("No auth token, unable to report panic"));
      return;
    }

    const auto &token = UNWRAP_REF(maybeToken);
    const auto panicData = (PanicData){
        msg,
        file,
        line,
        func,
    };

    const auto status = api.reportPanic(token, flash.readPlantId(), panicData);
    // TODO(pc): We could broadcast panics to other devices in the same network
    // if Api::reportPanic fails

    const uint32_t oneMinuteUs = 60 * 1000 * 1000;
    const uint32_t tenSecondsUs = 10 * 1000 * 1000;
    switch (status) {
    case ApiStatus::FORBIDDEN:
      logger.warn(F("Invalid auth token, but keeping since at panic"));
      return;

    case ApiStatus::NOT_FOUND:
      logger.warn(F("Invalid plant id, but keeping since at panic"));
      return;

    case ApiStatus::CLIENT_BUFFER_OVERFLOW:
      // TODO(pc): deal with this, but how? Truncating the msg?
      // Should we have an endpoint to report this type of error that can't
      // trigger it?
      return;

    case ApiStatus::BROKEN_SERVER:
      // Nothing we can do besides waiting.
      ESP.deepSleep(oneMinuteUs);
      continue;

    case ApiStatus::BROKEN_PIPE:
    case ApiStatus::TIMEOUT:
    case ApiStatus::NO_CONNECTION:
      // Nothing to be done besides retrying later
      ESP.deepSleep(tenSecondsUs);
      continue;

    case ApiStatus::OK:
      logger.info(F("Reported panic to server successfully"));
      return;

    case ApiStatus::MUST_UPGRADE:
      // TODO(pc): try to upgrade in here
      interruptEvent = InterruptEvent::MUST_UPGRADE;
      return;
    }
    const auto str = Network::apiStatusToString(status);
    logger.error(F("Unexpected status, panic.h: reportPanic: "), str);
  }
}

void panic__(const StringView msg, const StaticString file, const uint32_t line,
             const StringView func) {
  constexpr const uint16_t oneSecond = 1000;
  delay(oneSecond);
  const Log logger(CRIT, F("PANIC"));
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), msg);
  reportPanic(msg, file, line, func);
  WiFi.mode(WIFI_OFF);
  ESP.deepSleep(0);
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(const StaticString msg, const StaticString file,
             const uint32_t line, const StringView func) {
  constexpr const uint16_t oneSecond = 1000;
  delay(oneSecond);
  const Log logger(CRIT, F("PANIC"));
  logger.crit(F("Line "), std::to_string(line), F(" of file "), file,
              F(" inside "), func, F(": "), msg);
  WiFi.mode(WIFI_OFF);
  ESP.deepSleep(0);
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(const String &msg, const StaticString file, const uint32_t line,
             const StringView func) {
  panic__(msg, file, line, func);
}

void panic__(const std::string &msg, const StaticString file,
             const uint32_t line, const StringView func) {
  panic__(msg, file, line, func);
}

void panic__(const __FlashStringHelper *msg, const StaticString file,
             const uint32_t line, const StringView func) {
  panic__(msg, file, line, func);
}