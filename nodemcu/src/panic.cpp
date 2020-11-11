#include <panic.hpp>
#include <log.hpp>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

void panic__(const String msg, const StaticString file, const uint32_t line, const StaticString func) {
  panic__(StaticString(msg.c_str()), file, line, func);
}

void panic__(const StaticString msg, const StaticString file, const uint32_t line, const StaticString func) {
  delay(1000);
  const Log logger(CRIT, "PANIC");
  logger.crit(STATIC_STRING("Line"), START, STATIC_STRING(" "));
  logger.crit(String(line), CONTINUITY, STATIC_STRING(" "));
  logger.crit(STATIC_STRING("of file"), CONTINUITY, STATIC_STRING(" "));
  logger.crit(file, CONTINUITY, STATIC_STRING(", "));
  logger.crit(STATIC_STRING("inside"), CONTINUITY, STATIC_STRING(" "));
  logger.crit(func, CONTINUITY, STATIC_STRING(" "));
  logger.crit(STATIC_STRING(": "), CONTINUITY, STATIC_STRING(" "));
  logger.crit(msg, CONTINUITY);
  WiFi.mode(WIFI_OFF);
  // There should be a better way to black box this infinite loop
  while (EEPROM.read(0) != 3 && EEPROM.read(255) != 3) {
    yield();
  }
  __panic_func(file.get(), line, func.get());
}