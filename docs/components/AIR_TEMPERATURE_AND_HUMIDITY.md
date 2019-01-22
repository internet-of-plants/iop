# Air temperature and humidity

- DHT11 [*datasheet*](https://github.com/internet-of-plants/embedded/raw/master/docs/datasheets/DHT11.pdf)

   Humidity: 20% to 90%, accuracy +-4% (max +-5%)

   Temperature: 0ºC to 50ºC, accuracy +-2ºC

- DHT21 [*datasheet*](https://github.com/internet-of-plants/embedded/raw/master/docs/datasheets/DHT21%20(HM2301).pdf) ([*AM2301*](https://github.com/internet-of-plants/embedded/raw/master/docs/datasheets/AM2301.pdf))

   Humidity: 0% to 100%, accuracy +-3% (max +-5%)

   Temperature: -40ºC to 80ºC, accuracy +-1ºC

- DHT22 [*datasheet*](https://github.com/internet-of-plants/embedded/raw/master/docs/datasheets/DHT22%20(AM2303).pdf) ([*AM2302*](https://github.com/internet-of-plants/embedded/raw/master/docs/datasheets/AM2302.pdf))

   Humidity: 0% to 100%, accuracy +-2% (max +-5%)

   Temperature: -40ºC to 125ºC, accuracy +-0.2ºC

![Images of the DHT11, DHT21 and DHT22](https://raw.githubusercontent.com/internet-of-plants/embedded/master/docs/images/models/DHT11_DHT21_DHT22.png)

Other resources:

- [DHT.h lib docs](https://github.com/internet-of-plants/embedded/blob/master/docs/libs/DHT_FAMILY.md)
- https://playground.arduino.cc/Main/DHTLib

## Ports

1. VCC (3.3V to 5.5V power)

    *Sometimes 3.3V is not enough*

2. Digital data (DAT)

    Needs to be connected to VCC with a 4.7KOhm resistor between (medium-strength pull up)

    *Some models come with a built-in pull up resistor, so you don't need to put another, but it can't hurt*

3. Unused

    *Some models don't have this port exposed*

4. Ground (GND)

**Some models don't follow this pin order, check the datasheet, we have one, but it has the order printed in it ([photo](https://raw.githubusercontent.com/internet-of-plants/internet\_of\_plants/master/docs/images/wiring/DHT%20alternative.png))**

## Wiring

*Arduino*

![DHT wiring](https://raw.githubusercontent.com/internet-of-plants/embedded/master/docs/images/wiring/DHT.png)

## Information

- 2 seconds is the minimum interval between measurements (for DHT11 it's 1 second)

- Measurements may be unstable for up to 1 second after powering-up (a 100nF capacitor between VCC and GND solves that)

- Cables longer than 20 meters need a lower pull-up resistor, 2KOhm have been reported to work

   *More information at:* ***https://www.maximintegrated.com/en/app-notes/index.mvp/id/148***

## Source Code

Download the [DHT](https://github.com/adafruit/DHT-sensor-library) library and [add it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries)

More examples at: https://github.com/adafruit/DHT-sensor-library/tree/master/examples/DHTtester

```
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

    heat_index_celsius = dht.computeHeatIndex(temperature_celsius, humidity, /*isFahreinheit*/ false);
    heat_index_fahreinheit = dht.computeHeatIndex(temperature_fahreinheit, humidity);

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

Other methods that may help:

// If force is true the 2 seconds cache is bypassed
// DHT11 supports a 1 second interval
// But in general be careful when forcing the read
float readTemperature(bool force);

float convertCtoF(float celsius);
float convertFtoC(float fahreinheinheit);
```
