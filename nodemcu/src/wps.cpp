#include <wps.hpp>
#include <utils.hpp>
#include <configuration.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

volatile unsigned long wpsStateTime = 0; 

void wpsButtonDown() {
  Log().info("Pressed WPS button. Keep it pressed for at least 4 seconds to enable WPS mode");
  detachInterrupt(digitalPinToInterrupt(wpsButton));
  attachInterrupt(digitalPinToInterrupt(wpsButton), wpsButtonDown, FALLING);
  wpsStateTime = millis();
}

void wpsButtonUp() {
  detachInterrupt(digitalPinToInterrupt(wpsButton));
  attachInterrupt(digitalPinToInterrupt(wpsButton), wpsButtonUp, RISING);
  if (wpsStateTime + 4000 < millis()) {
    Log().debug("Setting WPS flag, enabling it in the next loop run");
    interruptEvent = WPS;
  }
}