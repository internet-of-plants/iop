# Sensors

1. [Air temperature](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#1-2---air-temperature-and-humidity)
2. [Air humidity](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#1-2---air-temperature-and-humidity)
3. [Soil temperature](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#3---soil-temperature)
4. [Soil resistivity](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#4---soil-resistivity)
5. [Light](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#5---light)

## 1, 2 - Air temperature and humidity

- DHT11 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/raw/master/doc/datasheets/DHT11.pdf)
- DHT21 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT21%20(HM2301).pdf) ([*AM2301*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2301.pdf))
- DHT22 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT22%20(AM2303).pdf) ([*AM2302*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2302.pdf))

![Images of the DHT11, DHT21 and DHT22](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/sensors/DHT11_DHT21_DHT22.png)

### Ports

1. VCC (3.3V or 5V power)

    *Sometimes 3.3V is not enough*

2. Digital data

    *Needs to be connected to VCC with a 4.7KOhm resistor between (medium-strength pull up)*

    *Some models come with a built-in pull up resistor, so you don't need to put another, but it can't hurt*

3. Unused

    *Some models don't have this port exposed*

4. Ground

**Some models don't follow this pin order, they are rare, but if it doesn't work like this it may be it**

**We have one, but the correct wiring is written in the sensor, so are most of the exceptions ([photo](https://raw.githubusercontent.com/internet-of-plants/internet\_of\_plants/master/doc/wiring/DHT%20with%20different%20wiring.png))**

### Wiring

*Arduino*

![DHT wiring (as described above)](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/wiring/DHT%20wiring.png)

### Source Code

   Download the [DHT](https://github.com/adafruit/DHT-sensor-library) library and [add it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries) (or place DHT.cpp and DHT.h in the source's folder)

   More examples at: https://github.com/adafruit/DHT-sensor-library/tree/master/examples/DHTtester

    #include "DHT.h"

    #define DHT_PIN 2      // Digital pin connected to data
    #define DHT_TYPE DHT11 // allow DHT11, DHT21, DHT22 (use DHT22 for AM2302) and AM2301

    DHT dht(DHT_PIN, DHT_TYPE);

    float temperature_celsius = 0;
    float temperature_fahreinheit = 0;

    float humidity_percentage = 0;

    float heat_index_celsius = 0;
    float heat_index_fahreinheit = 0;

    void setup() {
        Serial.begin(9600);

        dht.begin();
    }

    void loop() {
        temperature_celsius = dht.readTemperature();
        temperature_fahreinheit = dht.readTemperature(/*isFahreinheit*/ true);

        humidity_percentage = dht.readHumidity();

        if (isnan(temperature_celsius) || isnan(temperature_fahreinheit) || isnan(humidity_percentage)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        heat_index_celsius = dht.computeHeadIndex(temperature_celsius, humidity, /*isFahreinheit*/ false);
        heat_index_fahreinheit = dht.computeHeadIndex(temperature_fahreinheit, humidity);

        Serial.print("Humidity: ");
        Serial.println(humidity);
        Serial.print("Temperature: ");
        Serial.print(temperature_celsius);
        Serial.print(" *C, ");
        Serial.print(temperature_fahreinheit);
        Serial.println(" *F");
        Serial.print("Heat index: ");
        Serial.print(heat_index_celsius);
        Serial.print(" *C, ");
        Serial.print(heat_index_fahreinheit);
        Serial.println(" *F");
        Serial.println("-----------------------");

        // Most sensors need a 2 seconds delay, DHT11 allows 1 second
        // DHT.h library "forces" a 2 seconds delay by caching data
        // (you can bypass it, but be careful)
        delay(2000);
    }

## 3 - Soil temperature

- DS18B20 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/raw/master/doc/datasheets/DS18B20.pdf)
- DS18S20 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DS18S20.pdf)
- DS1820 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DS1820.pdf)
- DS1822 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DS1822.pdf)
- DS1825 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DS1825.pdf)
- DS28EA00 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DS28EA00.pdf)
- MAX31820 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/MAX31820.pdf)

![Images of the 3 types](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/sensors/DS18.png)

*Models may vary between these three types (waterproof case may also vary)*

### Ports

1. VCC (3.3V or 5V power) - **Optional**

    *OneWire protocol allows operation in parasite mode (without VCC)*

2. Digital data (OneWire pin)

    *Needs to be connected to VCC with a 4.7KOhm resistor between (medium-strength pull up)*

3. Ground

### Wiring

*Arduino*

![DS18B20 wiring (as described above)](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/wiring/DS18%20wiring.png)

*Example uses normal mode, parasite mode doesn't use the VCC pin, but needs the pull up in the OneWire bus anyway*

### Source Code

   Download the [OneWire](https://github.com/PaulStoffregen/OneWire) library and [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) add [it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries)

    #include "OneWire.h"
    #include "DallasTemperature.h"

    #define ONE_WIRE_BUS 3 // Sensor OneWire pin (digital data pin)

    OneWire one_wire(ONE_WIRE_BUS);

    // More than one sensor can use the same pin
    DallasTemperature sensors(&one_wire);

    float temperature_celsius = 0;
    float temperature_fahreinheit = 0;

    void setup() {
        Serial.begin(9600);

        sensors.begin();
    }

    void loop() {
        // Start temperature measurement in every sensor conencted to the OneWire bus
        // By default it blocks until temperature is ready to be read
        sensors.requestTemperatures();

        // Since there could be more than one sensor using the bus we need to access them by index
        temperature_celsius = sensors.getTempCByIndex(0);
        temperature_fahreinheit = sensors.getTempFByIndex(0);

        Serial.print("Temperature: ");
        Serial.print(temperature_celsius);
        Serial.print(" *C, ");
        Serial.print(temperature_fahreinheit);
        Serial.println(" *F");
        Serial.println("-----------------------");
    }

### Advanced Features

1. [Reading temperature using sensor's index is slow and not recommended](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#reading-temperature-by-index-is-slow-and-bad-read-by-address)
2. [Arduino can provide the sensor with energy, you may avoid using a VCC wire (parasite mode)](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#parasite-mode)
3. [You may plug many sensors in the same digital pin because of the OneWire bus protocol](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#use-one-digital-pin-to-manage-up-to-127-sensors)
5. [You may asynchronously start the temperature measurement to read it later (avoid always blocking until data is ready to be read)](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#asyncronously-measure-temperature-avoiding-blocking-while-data-is-not-ready)
6. [You may asynchronously start the temperature measurement in every sensor at the same time to read when needed (avoids blocking even more)](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#asynchronously-measure-temperature-in-every-sensor-in-onewire-bus-blocking-even-less)
7. [You can set the sensor's internal alarm to warn when temperature is too high or too low and check it when desired](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#set-sensors-internal-alarm-too-high-or-too-low-temperature-to-be-constantly-polled)
8. [You may change the sensor's temperature bit resolution (9, 10, 11 or 12 bits)](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#setting)
9. [You may use more than 127 sensors with OneWire by using more digital pins (each supports 127) and unifying them with the OneWire.h library](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#unify-two-onewire-buses-allows-more-than-127-sensors-to-be-controller-together-by-using-two-digital-pins)
10. [Store 6 bits of data in the sensor](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#store-6-bits-of-data-in-the-sensor-sensors-unused-configuration-storage---some-models-dont-have-this-space)
11. [Store 16 more bits of data in the sensor (can't be used if the alarm is enabled)](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#store-16-more-bits-in-the-sensor-sensors-alarm-configuration-storage-dont-enable-the-alarm-while-using-this-technique)

**TODO: explain more about more sensors needing a lower resistance in the data pull-up resistor**

#### Reading temperature by index is slow and bad, read by address

  Indexes are non-unique and depend on the context

   *Has to search (slow) the OneWire bus (index + 1) times to find the specified sensor*

  Get the device's unique, non-changeable, address and use it to identify it in the code

    // Address of a sensor (uint8_t array of 8 elements)
    DeviceAddress device_address;

    sensors.getAddress(device_address, index);

    // Sometimes data gets corrupted, assure it's the correct address
    // TODO: search more about this, I'm not sure it's exactly this description
    if (!sensors.validAddress(device_address)) {
        Serial.println("Address fetched was invalid, maybe try again?");
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

   **TODO: talk more about other methods that acess sensors by address**

    bool validAddress(const uint8_t*);
    bool validFamily(const uint8_t* deviceAddress);
    bool getAddress(uint8_t*, uint8_t);
    bool isConnected(const uint8_t*);
    bool isConnected(const uint8_t*, uint8_t*);
    bool readScratchPad(const uint8_t*, uint8_t*);
    void writeScratchPad(const uint8_t*, const uint8_t*);
    bool readPowerSupply(const uint8_t*);
    uint8_t getResolution(const uint8_t*);
    bool setResolution(const uint8_t*, uint8_t, bool skipGlobalBitResolutionCalculation = false);
    bool requestTemperaturesByAddress(const uint8_t*);
    int16_t getTemp(const uint8_t*);
    float getTempC(const uint8_t*);
    float getTempF(const uint8_t*);
    
#### Parasite mode

  **TODO: image of parasite mode wiring, example code (if needed - must check)**

  Doesn't use VCC pin

  Can't be polled in async mode since data-bus is powering the sensor

   *Needs assurance that the necessary time has passed before reading (block until needed time has passed right before reading - if necessary)*

   **If needed you may avoid blocking at all by trying to read many times during the code (hardware async sometimes is ugly)**

     // Allows asynchronous measurement
     sensors.setWaitForConversion(false);

     // Async start measuring, but data isn't ready yet
     sensors.requestTemperature();

     // Each resolution takes a different ammount of time to complete
     uint16_t now = millis();
     uint16_t until = now + millisToWaitForConversion(sensors.getResolution());

     // Some expensive work to do before the reading

     // Busy block for the missing time (instead of busy blocking for the entire time)
     while (now <= until);

     Serial.print("Temperature in celsius: ");
     Serial.println(sensors.getTempC(device_address)); // sensors.getTempCByIndex(0)

#### Use one digital pin to manage up to 127 sensors

  The OneWire protocol enables controlling up to 127 sensors using the same data bus (more if multiple data buses are unified)

   *More information about unifying buses in section:* ***[Unify two OneWire buses...](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#unify-two-onewire-buses-allows-more-than-127-sensors-to-be-controller-together-by-using-two-digital-pins)***

  It allows to talk to each sensor individually and to broadcast commands to every sensor using the bus

   *More information about individual sensor access in section:* ***[Reading temperature by index is slow and bad, read by address](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#reading-temperature-by-index-is-slow-and-bad-read-by-address)***

   *More information about broadcasting in section:* ***.***

  Each sensor has a unique, non-changeable, 64-bits address

   *More information in section* ***[Reading temperature by index is slow and bad, read by address](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#reading-temperature-by-index-is-slow-and-bad-read-by-address)***

  Counting devices using OneWire bus

    Serial.print("Numbers of devices using wire: ");
    Serial.println(sensors.getDeviceCount(), DEC);

    // Sensors DS18B20, DS18S20, DS1820, DS1822, DS1825, DS20EA00, MAX31820 accepted
    Serial.print("Numbers of devices of a 'validFamily' using wire: ");
    Serial.println(sensors.getDS18Count(), DEC);

   Finding out if bus needs parasite power (if at least one sensor needs it)

    Serial.print("Needs parasite power in OneWire bus: ");
    Serial.println(sensors.readPowerSupply());

    Serial.print("Cached data returned by sensors.readPowerSupply(): ");
    Serial.println(sensors.isParasitePowerMode());

  Manage the resolution of every device in the bus

##### Setting:

    // Very slow, since it talks individually to each sensor
    sensors.setResolution(9); // 9, 10, 11 or 12 (bits)

##### Getting

    Serial.print("Cached global resolution: ");
    Serial.println(sensors.getResolution());

#### Set sensor's internal alarm (too high or too low temperature) to be constantly polled

#### Configure temperature conversion bit resolution (9, 10, 11 or 12 bit resolutions)

#### Asyncronously measure temperature, avoiding blocking while data is not ready

   **TODO: finish**

   *Start the temperature measurement asynchronously, polling the value to read it later, instead of blocking when needed until it's ready*

   *Sensors in parasite mode can't be polled, wait the minimum necessary ammount of time before reading the value (do other work during conversion and block for the missing time - if needed - before reading the value)*

#### Asynchronously measure temperature in every sensor in OneWire bus, blocking even less

   **TODO: finish**

   *Ask every sensor to obtain the temperature asyncronously and read one by one when needed, avoiding necessarily blocking*

#### Unify two OneWire buses (allows more than 127 sensors to be controller together by using two digital pins)

#### Store 6 bits of data in the sensor (sensor's unused configuration storage - some models don't have this space)

#### Store 16 more bits in the sensor (sensor's alarm configuration storage, don't enable the alarm while using this technique)
