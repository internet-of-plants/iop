#include <panic.hpp>
#include <log.hpp>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <string_view.hpp>
#include <static_string.hpp>
#include <unsafe_raw_string.hpp>

#include <configuration.h>
#include <api.hpp>
#include <flash.hpp>

void reportPanic(const StringView & msg, const StaticString & file, const uint32_t line, const StringView & func) {
  const StaticString & host_ = host.asRef().expect(F("No host available: at panic__"));
  const Log logger(CRIT, F("PANIC"));
  const Api api(host_, TRACE);
  if (!api.isConnected()) {
    logger.crit(F("No connection, unable to report panic"));
    return;
  }
  const Flash flash(TRACE);
  const auto maybeToken = flash.readAuthToken();
  if (maybeToken.isNone()) {
    logger.crit(F("No auth token, unable to report panic"));
    return;
  }
  const AuthToken & token = maybeToken.asRef().expect(F("AuthToken is None but shouldn't: at panic__"));
  const auto panicData = (PanicData) { msg, file, line, func, };
  const auto resp = api.reportPanic(token, flash.readPlantId(), panicData);
  // TODO: We could broadcast panics to other devices in the same network if Api::reportPanic fails
  if (resp.isSome()) {
    const HttpCode & code = resp.asRef().expect(F("HttpCode is None but shouldn't: at panic__"));
    if (code == 200) {
      logger.info(F("Reported panic to server successfully"));
    } else {
      logger.crit(F("Api::reportPanic failed with http code "), START, F(" "));
      logger.crit(String(code), CONTINUITY);
    }
  } else {
    logger.crit(F("Api::reportPanic failed without a HttpCode"));
  }
}

void panic__(const StringView & msg, const StaticString & file, const uint32_t line, const StringView & func) {
  delay(1000);
  const String line_(line);
  const Log logger(CRIT, F("PANIC"));
  logger.crit(F("Line"), START, F(" "));
  logger.crit(line_, CONTINUITY, F(" "));
  logger.crit(F("of file"), CONTINUITY, F(" "));
  logger.crit(file, CONTINUITY, F(", "));
  logger.crit(F("inside"), CONTINUITY, F(" "));
  logger.crit(func, CONTINUITY, F(" "));
  logger.crit(F(": "), CONTINUITY, F(" "));
  logger.crit(msg, CONTINUITY);
  reportPanic(msg, file, line, func);
  WiFi.mode(WIFI_OFF);
  ESP.deepSleep(0);
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(const StaticString & msg, const StaticString & file, const uint32_t line, const StringView & func) {
  delay(1000);
  const Log logger(CRIT, F("PANIC"));
  logger.crit(F("Line"), START, F(" "));
  logger.crit(String(line), CONTINUITY, F(" "));
  logger.crit(F("of file"), CONTINUITY, F(" "));
  logger.crit(file, CONTINUITY, F(", "));
  logger.crit(F("inside"), CONTINUITY, F(" "));
  logger.crit(func, CONTINUITY, F(" "));
  logger.crit(F(": "), CONTINUITY, F(" "));
  logger.crit(msg, CONTINUITY);
  reportPanic(msg, file, line, func);
  WiFi.mode(WIFI_OFF);
  ESP.deepSleep(0);
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(const __FlashStringHelper * msg, const StaticString & file, const uint32_t line, const StringView & func) {
  panic__(StaticString(msg), file, line, func);
}