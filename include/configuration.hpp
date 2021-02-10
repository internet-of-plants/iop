#ifndef IOP_CONFIGURATION_H
#define IOP_CONFIGURATION_H

#include "core/log.hpp"
#include "core/option.hpp"
#include "utils.hpp"
#include <cstdint>


#include "pins_arduino.h"

/// Minimum log level to print a message (if serial is enabled)
const auto logLevel = iop::LogLevel::INFO;

const uint8_t soilTemperaturePin = D5;
const uint8_t airTempAndHumidityPin = D6;
const uint8_t soilResistivityPowerPin = D7;
const uint8_t factoryResetButton = D1;

/// Version of DHT (Digital Humidity and Temperature) sensor. (ex: DHT11 or
/// DHT21 or DHT22...)
const uint8_t dhtVersion = 22; // DHT22

/// Time between measurements
const esp_time interval = 180 * 1000;

/// Expands to:
/// static const char * const PROGMEM uri_progmem_char = <uri>;
/// static const iop::StaticString uri(uri_progmem_char);
///
/// It should be prefixed with https in production. Do not use plain http in
/// production!
///
/// But if using plain http, define IOP_NOSSL below, otherwise it won't work
/// PROGMEM_STRING(uri, "https://iop-monitor-server.tk:4001/v1");
PROGMEM_STRING(uri, "http://192.168.0.26:4001/v1");

/// Plain http doesn't work if IOP_NOSSL is not set
#define IOP_NOSSL

/// The fields bellow should be empty. Filling them will be counter productive
/// It's only here to speedup some debugging
///
/// If you really want to, replace the definitions with
/// MAYBE_PROGMEM_STRING(variableName, variableValue), like the variables above

static const iop::Option<iop::StaticString> wifiNetworkName;
static const iop::Option<iop::StaticString> wifiPassword;

static const iop::Option<iop::StaticString> iopEmail;
static const iop::Option<iop::StaticString> iopPassword;

#endif