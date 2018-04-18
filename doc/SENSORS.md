# Sensors

1. [Air temperature](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#1-2---air-temperature-and-humidity)
2. [Air humidity](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#1-2---air-temperature-and-humidity)
3. [Soil temperature](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#3---soil-temperature)
4. [Soil humidity](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#4---soil-humidity)
4. [Light](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#5---light)

## 1, 2 - Air temperature and humidity

- DHT11 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/raw/master/doc/datasheets/DHT11.pdf)
- DHT21 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT21%20(HM2301).pdf) ([*AM2301*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2301.pdf))
- DHT22 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT22%20(AM2303).pdf) ([*AM2302*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2302.pdf))

![Images of the 3 models](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/sensors/DHT11_DHT21_DHT22.png)

### Ports

1. VCC (3.3V or 5V power)

    *Sometimes 3.3V is not enough*

2. Digital data

    *Needs to be connected to VCC with a 10KOhm resistor between (medium-strength pull up)*

    *Some models come with a built-in pull up resistor, so you don't need to put another, but it can't hurt*

3. Unused

    *Some sensors don't have this port exposed*

4. Ground

### Wiring

*Arduino*

![DHT wiring (as described above)](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/wiring/DHT%20wiring.png)

### Source Code

   *Download the [DHT](https://github.com/adafruit/DHT-sensor-library) library and [add it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries) (or place DHT.cpp and DHT.h in the source's folder)*

    #include "DHT.h"

    #define DHT_PIN 2      // Digital pin connected to data
    #define DHT_TYPE DHT11 // allow DHT11, DHT21, DHT22 (use DHT22 for AM2302) and AM2301

    DHT dht(DHT_PIN, DHT_TYPE);

    float temperature_celcius = 0;
    float temperature_fahreinheit = 0;

    float humidity_percentage = 0;

    float heat_index_celcius = 0;
    float heat_index_fahreinheit = 0;

    void setup() {
        Serial.begin(9600);

        dht.begin();
    }

    void loop() {
        temperature_celcius = dht.readTemperature();
        temperature_fahreinheit = dht.readTemperature(/*isFahreinheit*/ true);

        humidity_percentage = dht.readHumidity();

        if (isnan(temperature_celcius) || isnan(temperature_fahreinheit) || isnan(humidity_percentage)) {
            Serial.println("Failed to read from DHT sensor!");
            return;
        }

        heat_index_celcius = dht.computeHeadIndex(temperature_celcius, humidity, /*isFahreinheit*/ false);
        heat_index_fahreinheit = dht.computeHeadIndex(temperature_fahreinheit, humidity);

        Serial.print("Humidity: ");
        Serial.println(humidity);
        Serial.print("Temperature: ");
        Serial.print(temperature_celcius);
        Serial.print(" *C, ");
        Serial.print(temperature_fahreinheit);
        Serial.println(" *F");
        Serial.print("Heat index: ");
        Serial.print(heat_index_celcius);
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

![Images of the 3 models](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/sensors/DS18.png)

*Models may vary between these three types (waterproof case may also vary)*

### Ports

1. VCC (3.3V or 5V power)

    *OneWire protocol allows for parasite mode (without VCC)*

2. Digital data (OneWire pin)

    *Needs to be connected to VCC with a 5KOhm resistor between*

3. Ground

### Wiring

*Arduino*

![DS18B20 wiring (as described above)](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/wiring/DS18%20wiring.png)

**Example uses normal mode, parasite mode doesn't use the VCC, but needs the pull up resistor in the OneWire bus anyway**

### Source Code

   *Download the [OneWire](https://github.com/PaulStoffregen/OneWire) library and [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) add [it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries)

    #include "OneWire.h"
    #include "DallasTemperature.h"

    #define ONE_WIRE_BUS 2 // Sensor OneWire pin (data pin)

    OneWire one_wire(ONE_WIRE_BUS);

    // More than one sensor can use the same pin
    DallasTemperature sensors(&one_wire);

    float temperature_celcius = 0;
    float temperature_fahreinheit = 0;

    void setup() {
        Serial.begin(9600);

        sensors.begin();
    }

    void loop() {
        // Ask all sensors connected to the OneWire bus to measure the temperature
        // You may also ask each sensor individually
        // By default it blocks until temperature is ready to be read
        sensors.requestTemperatures();

        // Since there could be more sensors we need to access them by a unique address retrieved beforehand, we can also access by index
        temperature_celcius = sensors.getTempCByIndex(0);
        temperature_fahreinheit = sensors.getTempFByIndex(0);

        Serial.print("Temperature: ");
        Serial.print(temperature_celcius);
        Serial.print(" *C, ");
        Serial.print(temperature_fahreinheit);
        Serial.println(" *F");
        Serial.println("-----------------------");
    }

### TODO

Extra:

    // Get the address of each sensor and use the bus for multiple sensors

    // You can start the measurement asynchronously and only read later, instead of blocking until it's ready to be read

    // You can avoid blocking even more by start measurement in every sensor at the same time asynchronously to read when needed (measurement doesn't take more than 750ms, so if you do other things for that time it won't block at all)

    // PS: getting the temperature by index is slow, you should get the address and use getTempC(address) instead
    //
    // DeviceAddress device_address;
    // getAddress(device_address, 0);
    // temperature_celcius = getTempC(device_address);
    // temperature_fahreinheit = getTempF(device_address);

#### Advanced Features

- Parasite mode

- Use one digital pin to communicate with up to 127 sensors

    *Each device plugged to the same digital pin has a unique 64-bits address to be accessed in the bus*

    *Before putting the sensor in the OneWire pin you have to get its address (plugging in the arduino and printing it)*

- Set sensor's internal alarm (too high or too low temperature) to be constantly polled

- Configure conversion (measurement) resolution (9, 10, 11 or 12 bit resolutions)

- Asyncronously request temperature

    *Ask sensor to measure the temperature, polling the value to get the measurement later, instead of blocking when needed until it's fetched*

- Request the temperature for all the sensors using the same pin

    *Ask every sensor to obtain the temperature asyncronously and read one by one when needed, avoiding necessarily blocking*

- Unify two OneWire buses (allows more than 127 sensors by using two ports)

- Use extra 6 bits in sensor's configuration register to store data

- Don't use internal alarms having extra 16 bits (two 8 bits registers) of general purpose storage
