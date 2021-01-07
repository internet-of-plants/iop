# Clock

- DS1302 [*datasheet*](/docs/datasheets/DS1302.pdf)

![Images of the DS1302](/docs/images/models/DS1302.png)

**Sometimes this model comes with problems in the internal crystal and voltage**

- DS3231 is more reliable

Other resources:

- https://playground.arduino.cc/Main/DS1302

## Ports

1. VCC (2V to 5.5V power)

2. Ground (GND)

3. Digital Clock (CLK)

4. Digital data (DTA)

5. Reset (RST)

## Information

- Binary representation of values is in BCD

- Has 12 and 24 hours mode (must reset time after change)

- Year: 2 digits, 0 is 2000, max (99) is 2099

- Takes months with less than 31 days into account

- Takes leap-years into account

- Does not follow Daylight Saving Time

- Has 31 bytes of general purpose volatile RAM (it will be lost when the battery ends)

- Allows for burst write operations

- Clock may be put on standby mode (low energy mode, with oscillator disabled)

## Wiring

*Arduino*

![DS1302 wiring](/docs/images/wiring/DS1302.png)

## Source Code

**TODO**
