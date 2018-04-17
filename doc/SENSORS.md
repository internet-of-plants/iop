# Sensors

1. [Air temperature](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/SENSORS.md#1-2---air-temperature-and-humidity)
2. Air humidity
3. Soil temperature
4. Soil humidity
5. Light

## 1, 2 - Air temperature and humidity

- DHT11 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/raw/master/doc/datasheets/DHT11.pdf)
- DHT21 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT21%20(HM2301).pdf) ([*AM2301*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2301.pdf))
- DHT22 [*datasheet*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/DHT22%20(AM2303).pdf) ([*AM2302*](https://github.com/internet-of-plants/internet_of_plants/blob/master/doc/datasheets/AM2302.pdf))

![Images of the 3 models](https://raw.githubusercontent.com/internet-of-plants/internet_of_plants/master/doc/sensors/DHT11_DHT21_DHT22.png)

### Ports

1. VCC (3.3V or 5V power)

    *Sometimes 3.3V is not enough*

2. Digital data out

    *Needs to be connected to VCC with a 10Kohm resistor between (medium-strength pull up)*

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

    #define DHT_PIN 2      // Digital pin data-out is connected to
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
