# Soil resistivity

- FC28 *Analog Sensor* + LM393 [*datasheet*](/docs/components/datasheets/LM393.pdf)

![Image of FC28 with LM393](/docs/components/images/models/FC28+LM393.png)

## Ports

*The port ordering is usually written in LM393*

1. A0 - Analogic data

2. D0 - Digital data

3. Ground (GND)

4. VCC (3.3V to 5V power)

## Information

- Sensor is made of 2 electrodes that generate analogic soil resistivity data

- Analogic data is the raw sensor's measurement

- Digital data is HIGH if value is above threshold specified in LM393's potentiometer


## Wiring

*Arduino*

-**Analogic (A0 to arduino's analogic port)** ![LM393 analog wiring](/docs/images/wiring/FC28+LM393-analog.png)

-**Digital (D0 to arduino's digital port)** ![LM393 digital wiring](/docs/images/wiring/FC28+LM393-digital.png)

## Source Code

 Download the [OneWire](https://github.com/PaulStoffregen/OneWire) library and [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) add [it to your Arduino IDE](https://www.arduino.cc/en/Hacking/Libraries)

```
```

## Advanced Features
