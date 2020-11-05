#include <wps.hpp>
#include <utils.hpp>
#include <configuration.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

volatile unsigned long wpsStateTime = 0; 

namespace wps {
  void setup() {
    attachInterrupt(digitalPinToInterrupt(wpsButton), wps::buttonDown, RISING);
  }

  void buttonDown() {
    detachInterrupt(digitalPinToInterrupt(wpsButton));
    attachInterrupt(digitalPinToInterrupt(wpsButton), wps::buttonUp, FALLING);
    wpsStateTime = millis();
    logger.info("Pressed WPS button. Keep it pressed for at least 4 seconds to enable WPS mode");
  }

  void buttonUp() {
    detachInterrupt(digitalPinToInterrupt(wpsButton));
    attachInterrupt(digitalPinToInterrupt(wpsButton), wps::buttonDown, RISING);
    if (wpsStateTime + 4000 < millis()) {
      interruptEvent = WPS;
      logger.info("Setted WPS flag, enabling it in the next loop run");
    }
  }
}
