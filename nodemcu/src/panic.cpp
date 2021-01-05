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

// TODO: save unique panics to flash
// https://github.com/sticilface/ESPmanager/blob/dce7fc06806a90c179a40eb2d74f4278fffad5b4/src/SaveStack.cpp
void reportPanic(const StringView &msg, const StaticString &file,
                 const uint32_t line, const StringView &func) {
  const auto &host_ = UNWRAP_REF(host);
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

  const auto &token = UNWRAP_REF(maybeToken);
  const auto panicData = (PanicData){
      msg,
      file,
      line,
      func,
  };

  const auto resp = api.reportPanic(token, flash.readPlantId(), panicData);
  // TODO: We could broadcast panics to other devices in the same network if
  // Api::reportPanic fails
  if (resp.isSome()) {
    const auto &code = UNWRAP_REF(resp);
    if (code == 200) {
      logger.info(F("Reported panic to server successfully"));

    } else {
      // If this request fails there is nothing we can do besides logging it
      logger.crit(F("Api::reportPanic failed with http code "),
                  std::to_string(code));
    }

  } else {
    // Nothing to be done if the connection failed during panic
    logger.crit(F("Api::reportPanic failed without a HttpCode"));
  }
}

void panic__(const StringView msg, const StaticString file, const uint32_t line,
             const StringView func) {
  delay(1000);
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
  delay(1000);
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