# 3 - Soil temperature

TODO: [error values](https://github.com/openenergymonitor/learn/blob/master/view/electricity-monitoring/temperature/DS18B20-temperature-sensing.md#software), [network design](https://github.com/openenergymonitor/learn/blob/master/view/electricity-monitoring/temperature/DS18B20-temperature-sensing.md#notes-and-further-reading)

- DS18B20 [*datasheet*](/docs/datasheets/DS18B20.pdf)
- DS18S20 [*datasheet*](/docs/datasheets/DS18S20.pdf)
- DS1820 [*datasheet*](/docs/datasheets/DS1820.pdf)
- DS1825 [*datasheet*](/docs/datasheets/DS1825.pdf)

   Temperature of the above: -55ºC to 125ºC, accuracy +-0.5ºC

- DS28EA00 [*datasheet*](/docs/datasheets/DS28EA00.pdf)

   Temperature: -40ºC to 85ºC

- DS1822 [*datasheet*](/docs/datasheets/DS1822.pdf)
- MAX31820 [*datasheet*](/docs/datasheets/MAX31820.pdf)

   Temperature of the above: -55ºC to 125ºC, accuracy +-2ºC

![Images of the 3 types](/docs/images/models/DS18.png)

*Models may vary between these three types (waterproof case may also vary)*

Other resources:

- [DallasTemperature.h lib docs](/docs/libs/DS18_FAMILY.md)
- [OneWire.h lib docs](/docs/libs/DS18_FAMILY.md)
- https://playground.arduino.cc/Learning/OneWire
- https://github.com/openenergymonitor/learn/master/view/electricity-monitoring/temperature/DS18B20-temperature-sensing.md
- https://github.com/openenergymonitor/learn/master/view/electricity-monitoring/temperature/DS18B20-temperature-sensing-2.md

## Ports

1. VCC/VDD (3.3V to 5.5V power) - **Optional**

    *OneWire protocol allows operation in parasite mode (without VCC)*

2. Digital data (OneWire pin) (DAT)

    *Needs to be connected to VCC with a 4.7KOhm resistor between (medium-strength pull up)*

3. Ground (GND)

## Wiring

*Arduino*

![DS18B20 wiring in normal and parasite mode](/docs/images/wiring/DS18.png)

## Information

**More information in Advanced section**

- Each sensor has a unique 64 bits address (ROM)

- Sensor can be operated in parasite mode (no VCC)

- Multiple sensors can be plugged into the same digital pin

- Can measure the temperature in many sensors at the same time

- Can asynchronously measure the temperature

- Has custom conversion bit resolution (9, 10, 11 or 12 bits)

   DS18S20 and DS1820 have fixed 9 bits resolution

- Has internal alarms (too high, too low)

   DS18S20 and DS1820 don't have alarms

- If alarm is disabled there are 16 bits of free storage in each sensor

   DS18S20 and DS1820 don't have alarms

## Source Code

 Download the [OneWire](https://github.com/PaulStoffregen/OneWire) library and [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) add [it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries)

```
#include "OneWire.h"
#include "DallasTemperature.h"

#define ONE_WIRE_BUS 3 // OneWire pin (digital data pin)

OneWire one_wire(ONE_WIRE_BUS);

// More than one sensor can be connected to the same pin
DallasTemperature sensors(&one_wire);

float temperature_celsius = 0;
float temperature_fahreinheit = 0;

void setup() {
    Serial.begin(9600);

    sensors.begin();
}

void loop() {
    // Start temperature measurement in every sensor conenected to the OneWire bus
    // By default it blocks until temperature is ready to be read
    sensors.requestTemperatures();

    // Since there could be more than one sensor using the bus we need to access them by index (or address - recommended)
    temperature_celsius = sensors.getTempCByIndex(0);
    temperature_fahreinheit = sensors.getTempFByIndex(0);

    Serial.print("Temperature: ");
    Serial.print(temperature_celsius);
    Serial.print(" *C, ");
    Serial.print(temperature_fahreinheit);
    Serial.println(" *F");
    Serial.println("-----------------------");
}
```

### Advanced Features

1. [Reading temperature by sensor's index is slow and not recommended, read by address](#reading-temperature-by-sensors-index-is-slow-and-not-recommended-read-by-address)
2. [Parasite mode - Arduino can provide the sensor with energy instead of using a VCC wire](#parasite-mode)
3. [Plug many sensors in the same digital pin because of the OneWire bus protocol](#use-one-digital-pin-to-manage-up-to-127-sensors)
4. [Set sensor's internal alarms (too high and too low) to be polled](#set-sensors-internal-alarms-too-high-and-too-low-to-be-polled)
5. [Change the sensor's temperature bit resolution (9, 10, 11 or 12 bits)](#configure-temperature-conversion-bit-resolution)
6. [Asynchronously measure the temperature, to read later](#asyncronously-measure-the-temperature)
7. [Store 16 bits of data in the sensor (can't be used if sensor's alarm is enabled)](#store-16-bits-of-data-in-the-sensor-cant-be-used-if-sensors-alarm-is-enabled)
8. [Extra: Dynamically search every digital pin for a OneWire bus](#extra-dynamically-search-every-digital-pin-for-a-onewire-bus)

### Reading temperature by sensor's index is slow and not recommended, read by address

Indexes are non-unique and depend on the context

To access each index (slow) the library searches the bus (index + 1) times to find the specified sensor

Get the device's unique, non-changeable, address and use it to identify it in the code

Accessing sensors by address example:

```
// Address of a sensor (uint8_t array of 8 elements)
DeviceAddress device_address;

// Select sensor's address by index
sensors.getAddress(device_address, index);

// Sometimes data gets corrupted, assure it's the correct address
if (!sensors.validAddress(device_address)) {
    Serial.println("Address fetched was invalid, maybe try again?");
    return
}

// Attemps to assure device is connected
if (!sensors.isConnected(device_address)) {
    Serial.println("Failed to connected with device");
    return
}

float temperature_celsius = sensors.getTempC(device_address);
float temperature_fahreinheit = sensors.getTempF(device_address);

// Save the displayed address and use in the code
// Example output: Device's address: 0x28 0x1D 0x39 0x31 0x2 0x0 0x0 0xF0
// Accessing in real code:
//     DeviceAddress device_address = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
//     float temperature_celsius = sensors.getTempC(device_address);
//     float temperature_fahreinheit = sensors.getTempF(device_address);

Serial.println("Device's address: ");
for (uint8_t i = 0; i < 8; ++i) {
    Serial.print(device_address[i], HEX);
    Serial.print(" ");
}
```

Other useful methods to access a sensor by address:

```
// Assures sensor is from a known family (DS18B20, DS18S20, DS1822, DS1825 or DS28EA00)
bool validFamily(const uint8_t* deviceAddress);

// Attempt to discover if sensor is connected
bool isConnected(const uint8_t* deviceAddress);

// Attempt to discover if sensor is connected (fills ScratchPad)
bool isConnected(const uint8_t* deviceAddress, uint8_t* scratchPad);

// Fills scratchPad
bool readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad);

// Write scratchPad to sensor
void writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad);

// Return true if device uses parasite power
bool readPowerSupply(const uint8_t* deviceAddress);

// Get conversion resolution to device (in D18S20 and DS1820 resolution is fixed at 9 bits)
uint8_t getResolution(const uint8_t* deviceAddress);

// Save conversion resolution to device (in DS18S20 and DS1820 resolution is fixed at 9 bits)
bool setResolution(const uint8_t* deviceAddress, uint8_t resolution, bool skipGlobalBitResolutionCalculation = false);

// Start temperature measurement in sensor
bool requestTemperaturesByAddress(const uint8_t* deviceAddress);

// Return raw temperature (12 bit integer of 1/128 degrees C)
int16_t getTemp(const uint8_t* deviceAddress);

// Sets the high alarm temperature for a device (-55*C to 125*C)
void setHighAlarmTemp(const uint8_t* deviceAddress, int8_t temperature);

// Sets the low alarm temperature for a device (-55*C to 125*C)
void setLowAlarmTemp(const uint8_t* deviceAddress, int8_t temperature);

// Returns the high alarm temperature for a device (-55*C to 125*C)
int8_t getHighAlarmTemp(const uint8_t* deviceAddress);

// Returns the low alarm temperature for a device (-55*C to 125*C)
int8_t getLowAlarmTemp(const uint8_t* deviceAddress);

// Search the bus for devices with active alarms (internal)
bool alarmSearch(uint8_t* deviceAddress);

// Returns true if a specific device has an alarm
bool hasAlarm(const uint8_t* deviceAddress);

// If no alarm is set there will be 16 bits available of memory in each sensor
void setUserData(const uint8_t* deviceAddress, int16_t data);
int16_t getUserData(const uint8_t* deviceAddress);
```

### Parasite mode

Doesn't use VCC pin

Can't be polled in async mode since data-bus is powering the sensor

- Needs assurance that the necessary time has passed before reading (block until needed time has passed right before reading - if necessary)

Parasite example:

```
// Disables busy wait during conversion
// (all requests will be async, turn on before a request that must be sync)
sensors.setWaitForConversion(false);

// Async start measurement, but data isn't ready yet
sensors.requestTemperatures();

// Each conversion resolution takes a different ammount of time to complete
uint16_t until = millis() + millisToWaitForConversion(sensors.getResolution());

/* Some expensive work to do before the reading */

// Busy block for the missing time (instead of busy blocking for the entire time)
while (millis() <= until);

Serial.print("Temperature in celsius: ");
Serial.println(sensors.getTempC(device_address));
```

**If needed you may avoid blocking at all by constantly checking if the necessary time has passed while doing other things (hardware async sometimes is ugly)**

**Don't set checkForConversion as false in this mode, otherwise the function 'blockTillConversionComplete(uint8_t bitResolution)' will try to poll the sensor, which is impossible in parasite mode**

**Above 100ºC communication may fail in parasite mode, an external supply is recommended**

### Use one digital pin to manage up to 127 sensors

The OneWire protocol enables controlling up to 127 sensors using the same data bus

It allows to talk to each sensor individually and to broadcast commands to every sensor using the bus

- More information about individual sensor access in section **[Read by address](#reading-temperature-by-sensors-index-is-slow-and-not-recommended-read-by-address)**

Each sensor has a unique, non-changeable, 64-bits address

- More information in section **[Read by address](#reading-temperature-by-sensors-index-is-slow-and-not-recommended-read-by-address)**

Counting devices using OneWire bus

```
Serial.print("Numbers of devices using wire: ");
Serial.println(sensors.getDeviceCount(), DEC);

// Sensors DS18B20, DS18S20, DS1820, DS1822, DS1825, DS20EA00, MAX31820 accepted
Serial.print("Numbers of devices of a 'validFamily' using wire: ");
Serial.println(sensors.getDS18Count(), DEC);
```

Finding out if bus needs parasite power (if at least one sensor needs it)

```
Serial.print("Needs parasite power in OneWire bus: ");
Serial.println(sensors.readPowerSupply());

Serial.print("Cached data returned by sensors.readPowerSupply(): ");
Serial.println(sensors.isParasitePowerMode());
```

Manage the resolution of every device in the bus

- The higher the resolution the longer it takes to convert (most sensors default to 12)

- D18S20 and DS1820 have fixed resolutions (9 bits)

1. Setting:

```
// Very slow, since it talks individually to each sensor
sensors.setResolution(9); // 9, 10, 11 or 12 (bits)
```

2. Getting

```
Serial.print("Cached global resolution: ");
Serial.println(sensors.getResolution());
```

### Set sensor's internal alarms (too high and too low) to be polled

D18S20 and DS1820 don't have alarms

If alarms are not used there will be 16 extra free bits in each sensor

- More information in section: **[Store 16 bits in the sensor](#store-16-more-bits-of-data-in-the-sensor-cant-be-used-if-sensors-alarm-is-enabled)**

Alarm management example:

```
void alarm_handler(const uint8_t* deviceAddress) {
    Serial.print("ALARM: ");
    Serial.print(serial.getTempC(deviceAddress));
    Serial.println("*C (alarm_handler)");
}

void setup() {
    // In celsius (-55 to 125*C)
    sensors.setLowAlarmTemp(device_address, 15);
    sensors.setHighAlarmTemp(device_address, 50);

    sensors.setAlarmHandler(&alarm_handler);
    // assert(sensors.hasAlarmHandler() == true);

    // get*AlarmTemp will return DEVICE_DISCONECTED if sensor is not found
    // Alarm temp: low = 15, high = 50
    Serial.print("Alarm temp: low = ");
    Serial.print(sensors.getLowAlarmTemp(device_address));

    Serial.print(", high = ");
    Serial.println(sensors.getHighAlarmTemp(device_address));
}

void loop() {
    sensors.requestTemperatures();

    // Uses data from last 'requestTemperatures' to check if there is an alarm
    if (sensors.hasAlarm(device_address)) {
        Serial.print("ALARM: ");
        Serial.print(serial.getTempC(deviceAddress));
        Serial.println("*C (hasAlarm));
    }

    // Will call alarm_handler if there is an alarm (ask each sensor if alarm was triggered)
    sensors.processAlarms();
}
```

Internal, but exposed, methods related to alarms

```
// Search the wire for devices with active alarms
bool alarmSearch(uint8_t*);

// Resets internal variables used for the alarm search
void resetAlarmSearch(void);
```

### Configure temperature conversion bit resolution

The higher the resolution the longer it takes to convert (most sensors default to 12)

D18S20 and DS1820 have fixed resolutions (9 bits)

1. Setting:

```
// Set specific sensor's resolution
sensors.setResolution(device_address, 9);

// Set every sensor's resolution, but it's slow
// Talks with each sensor individually
sensors.setResolution(9); // 9, 10, 11 or 12 (bits)
```

2. Getting

```
Serial.print("Sensor's resolution: ");
Serial.println(sensors.getResolution(device_address));

Serial.print("Cached global resolution: ");
Serial.println(sensors.getResolution());
```

### Asyncronously measure the temperature

Start the temperature measurement asynchronously in every sensor (broadcast)

Poll the value to read it/them later, instead of blocking when needed until it's ready

- Can be constantly polled during the code or just block when needed to read (after some other work has been complete)

Sensors in parasite mode can't be polled, wait the minimum necessary ammount of time before reading the value

- More information in section: **[Parasite mode](#parasite-mode)**

Async example:

```
#include<OneWire.h>
#include "DallasTemperature.h"

#define ONE_WIRE_BUS 3 // OneWire pin (digital data pin)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(9600);

    sensors.begin();

    // Makes every requestTemperatures async (should be re-enabled for a sync requestTemperatures)
    sensors.setWaitForConversion(false);
}

void loop() {
    sensors.requestTemperatures();

    // Each conversion resolution takes a different ammount of time to complete
    uint16_t until = millis() + millisToWaitForConversion(sensors.getResolution());

    /* Some expensive work to do before the reading */

    // Poll first sensor in bus to check if conversion is complete
    // Other sensors may not be ready (is that a problem with many sensors?)
    while (!sensors.isConversionComplete() && millis() <= until);

    Serial.print("Temperature in celsius: ");
    Serial.println(sensors.getTempCByIndex(0);
}
```

### Store 16 bits of data in the sensor (can't be used if sensor's alarm is enabled)

Use sensor's alarm configuration storage

DS18S20 and DS1820 don't have this registers

```
// Stores uint16_t in specified device
// Doesn't assure it's written (if something happens it may not write)
// sensors.setUserDataByIndex(0, 200);
sensors.setUserData(device_address, 200);

// User data: 200
Serial.print("User data: ");
Serial.println(sensors.getUserData(device_address));
// Serial.println(sensors.getUserDataByIndex(0));
```

### Extra: Dynamically search every digital pin for a OneWire bus

**Not really recommended, but possible**

```
#include<OneWire.h>
#include "DallasTemperature.h"

#define MAX_PORTS 20

uint8_t buses = 0;
uint8_t pins[MAX_PORTS] = {};
DallasTemperature sensors_buses[MAX_PORTS] = {};
uint8_t devices_count[MAX_PORTS] = {};

void setup() {
    Serial.begin(9600);

    for (uint8_t pin = 0; pin < MAX_DEVICES; ++pin) {
        OneWire ow(pin);
        uint8_t address[8];

        // If a OneWire device is found its 'address' is filled with its address
        // If no device is found returns false
        if (ow.search(address)) {
            pins[buses] = pin;
            sensors_buses[buses] = DallasTemperature(ow);
            devices_count[buses] = sensors_buses[buses].getDeviceCount();
            ++buses;
        }
    }
}

void loop() {
    // In async mode you may request the temperatures for each bus before, in another loop
    // And wait the necessary time before reading, instead of blocking completely for each bus

    for (uint8_t bus = 0; bus < buses; ++index) {
        // Start the temperature measurement in every sensor conenected to this OneWire bus
        // By default it blocks until this bus temperatures are ready to be read
        sensors_buses[bus].requestTemperatures();

        Serial.print("Sensors for pin ");
        Serial.print(pins[bus]);
        Serial.println(":");

        for (uint8_t device = 0; device < devices_count[bus]; ++device) {
            Serial.print("  Sensor ");
            Serial.println(device);
            Serial.println(":");

            // Very slow, should get temp by address
            temperature_celsius = sensors.getTempCByIndex(device);
            temperature_fahreinheit = sensors.getTempFByIndex(device);

            Serial.print("    Temperature: ");
            Serial.print(temperature_celsius);
            Serial.print(" *C, ");
            Serial.print(temperature_fahreinheit);
            Serial.println(" *F");
            Serial.println("-----------------------");
        }
    }
}
```
