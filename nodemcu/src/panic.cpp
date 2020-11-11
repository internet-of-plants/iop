#include <panic.hpp>
#include <log.hpp>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WString.h>

void panic__(const StringView msg, const StaticString file, const uint32_t line, const char * const func) {
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
  // There should be a better way to black box this infinite loop
  while (EEPROM.read(0) != 3 && EEPROM.read(255) != 3) {
    yield();
  }
  __panic_func(file.get(), line, func);
}

void panic__(const StaticString msg, const StaticString file, const uint32_t line, const char * const func) {
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
  // There should be a better way to black box this infinite loop
  while (EEPROM.read(0) != 3 && EEPROM.read(255) != 3) {
    yield();
  }
  __panic_func(file.get(), line, func);
}