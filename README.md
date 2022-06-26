# Internet of Plants - High Level Embedded Framework

High Level Modern C++17 Framework for embedded systems. Opinionated and safer version of [iop-hal](https://github.com/internet-of-plants/iop-hal).

Integrated with [internet-of-plants/server](https://github.com/internet-of-plants/server).

## Targets supported

- ESP8266 (all boards supported by [esp8266/Arduino](https://github.com/esp8266/Arduino))
- ESP32 (all boards supported by [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32/))
- Posix (all targets that support POSIX)

*Note: Some functionalities in the posix target are NOOP, most will be implemented to support Raspberry Pis and other boards that support posix. But for now this is mostly used for testing*

## Features

- All features from [iop-hal](https://github.com/internet-of-plants/iop-hal)
    - Don't use iop-hal's entrypoints, use the setup entrypoint + the task system below
- [`iop::setup`](https://github.com/internet-of-plants/iop/blob/main/include/iop/loop.hpp): User defined `iop` entrypoint, from `#include <iop/loop.hpp>`
- [`iop::Api`](https://github.com/internet-of-plants/iop/blob/main/include/iop/api.hpp): Abstracts [internet-of-plants/server](https://github.com/internet-of-plants/server)'s API, from `#include <iop/api.hpp>`
    - Unauthenticated: login
    - Authenticated: send measurements, register log, report panic, over the air upgrade
- Network logging
- Panics wait for updates instead of just halting
- [`iop::Storage`](https://github.com/internet-of-plants/iop/blob/main/include/iop/storage.hpp): High level authentication persistance management, from `#include <iop/storage.hpp>`
    - Persistance of [internet-of-plants/server](https://github.com/internet-of-plants/server)'s authentication token
    - Persistance of WiFi credentials
- [`iop::CredentialsServer`](https://github.com/internet-of-plants/iop/blob/main/include/iop/server.hpp): Captive portal to log into WiFI and IoP account, from `#include <iop/server.hpp>`
- [`iop::EventLoop::{setAuthenticatedInterval, setInterval}`](https://github.com/internet-of-plants/iop/blob/main/include/iop/loop.hpp): Recurrent task system registry, from `#include <iop/loop>`

## Integrated Sensors

- [Dallas Temperature](https://github.com/internet-of-plants/dallas_temperature)
    - DS18B20, DS18S20, DS1822, DS1820
- [DHT](https://github.com/internet-of-plants/dht)
    - DHT11, DHT12, DHT21, DHT22, AM2301
- [Factory Reset Button](https://github.com/internet-of-plants/factor_reset_button)
    - Any digital input that works similar to a button
- [Soil Resistivity](https://github.com/internet-of-plants/soil_resistivity)

## Managing it from [internet-of-plants/server](https://github.com/internet-of-plants/server)

You might not want to use this library directly as it can be easier to configure your device from [internet-of-plants/server](https://github.com/internet-of-plants/server).

There you can specify the target and configure the sensors in the appropriate ports. With that the [internet-of-plants/server](https://github.com/internet-of-plants/server) will generate the C++ code for you and compile it, providing an automatic over the air update. And being able to track metadata for each event. This means you never have to touch a piece of C++ nor build system to compile it.

This works for almost all cases, but if you want to customize it in ways the user interface doesn't allow you to, use this as a Platform IO library.

## How it works

When the device has a WiFi connection and has the appropriate credentials it will authenticate with the [internet-of-plants/server](https://github.com/internet-of-plants/server). You will be able to see it with [internet-of-plants/client](https://github.com/internet-of-plants/client). There you will be able to configure it properly, to provide updates and properly monitor its measurements, logs and panics. Devices can be grouped to be managed at scale.

If it isn't connected to a WiFi Access Point, or it isn't authenticated to [internet-of-plants/server](https://github.com/internet-of-plants/server), it will open its own WiFi Access Point with a Captive Portal that will collect the WiFi and IoP credentials. For now the Access Point will have a hardcoded credential, which is a liability, eventually we want to make this dynamic.

When the credentials are supplied it will authenticate itself with the WiFi Access Point and then make a login request to [internet-of-plants/server](https://github.com/internet-of-plants/server). The IoP credentials aren't stored, they are just used to obtain an authentication token for that device from the server, the token then is stored (if the authentication succeeds).

After the authentication it will periodically run the attributed authenticated and unauthenticated tasks. They generally will collect measurements and then register it to [internet-of-plants/server](https://github.com/internet-of-plants/server).

It will also send every log with a level of at least INFO to the [internet-of-plants/server](https://github.com/internet-of-plants/server) (if the filter level is INFO or lower), so you can keep track of the device as it runs.

If the monitor server provides a firmware update, the next time the device sends a measurement to the server it will schedule the upgrade, the update will be requested from the monitor server and after it's presisted the device will be rebooted and start running the new version (the bootloader will replace the versions in a power-loss resistant way).

Eventually the updates will demand signed binaries. Binary compression also is possible with gzip.

If some critical problem happens (the panic machinery is called) it will be reported to the server and the device will await for a upgrade from [internet-of-plants/server](https://github.com/internet-of-plants/server).

If there is no network available the device will halt forever and will need to be restarted/updated physically (through the serial port).

TODO: log stack traces and log reboots from crashes and whatever data we can recover from the crash
TODO: implement tooling that makes firmware compression out of the box.

## Dependencies

PlatformIO

Needs OpenSSL in PATH, clang-format and clang-tidy (install LLVM to get it). Be nice to our codebase :).

- https://wiki.openssl.org/index.php/Binaries
- https://releases.llvm.org/download.html

On linux you have to change permissions to be able to deploy to a serial port

```
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```

## Example

https://github.com/internet-of-plants/example-firmware

## TODO

Grep for "TODO"'s to find the known missing pieces