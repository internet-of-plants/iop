#include <panic.hpp>
#include <log.hpp>
#include <ESP8266WiFi.h>
#include <EEPROM.h>

void panic__(const String msg, const String file, const uint32_t line, const String func) {
  delay(1000);
  const Log logger(CRIT, "PANIC");
  logger.crit("Line " + String(line) + " of file " + file + ", inside " + func + ": " + msg);
  WiFi.mode(WIFI_OFF);
  // There should be a better way to black box this infinite loop
  while (EEPROM.read(0) != 3 && EEPROM.read(255) != 3) {
    yield();
  }
  __panic_func(file.c_str(), line, func.c_str());
}