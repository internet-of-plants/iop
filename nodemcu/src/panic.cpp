#include <panic.hpp>
#include <log.hpp>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

#include <string_view.hpp>
#include <static_string.hpp>
#include <unsafe_raw_string.hpp>

// TODO: report errors to monitor if there is internet access
// (in the future we could also broadcast to other devices in the same network)

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
  WiFi.mode(WIFI_OFF);
  ESP.deepSleep(0);
  __panic_func(file.asCharPtr(), line, func.get());
}

void panic__(const __FlashStringHelper * msg, const StaticString & file, const uint32_t line, const StringView & func) {
  panic__(StaticString(msg), file, line, func);
}