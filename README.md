# Internet of Plants - Embedded Software

Firmware for Internet of Plants embedded system. Made for ESP8266.

Features:

Collects data from sensors and reports them to the monitor server, a TLS encrypted HTTP/1.x server, in JSON format. The code for the server can be found here: [https://github.com/internet-of-plants/server](https://github.com/internet-of-plants/server)

When the device is online and authenticated to the server a new plant is automatically created on your account if it doesn't already exist. Now you are able to track your plant with the monitor server. There you also will be able to provide updates to each specific device (or a group of them, or all of them). And monitor panics + logs for those devices.

If it isn't connected to a WiFi Access Point, or it isn't authenticated to the monitor server, it will open its own WiFi Access Point with a captive portal that will collect the WiFi and IoP credentials.

When the credentials are supplied it will authenticate with the WiFi Access Point and then make a login request to the monitor server. The IoP credentials aren't stored in storage, they are used to obtain an authentication token for that plant from the server, the token then is stored (if the authentication succeeds).

After the authentication it will periodically collect sensors data and report the events to the monitor server.

It will also send every log with a criticality of at least INFO to the monitor server, so you can keep track of the device as it runs.

If the monitor server provides a firmware update, the next time the device talks with the server it will schedule a download (probably when sending the logs of the incomming monitoring event), the update will be requested from the monitor server and after it's stored in storage the device will be rebooted and start running the new version (the bootloader will replace the versions in a power-loss resistant way).

If some critical problem happens (the panic function is called) it will be reported to the server and the device will constantly request the monitor server for updates.

TODO: log stack traces and log reboots from crashes and whatever data we can recover from the crash

# Monitoring

Each monitoring event represents a fixed set of measurements containing: air humidity, air temperature, soil temperature and soil humidity. We intent to expand this eventually to support custom sensors and to allow for batch monitoring (like a greenhouse). But it's not there yet.

This is a monitoring system by design. One day it might automate tasks, but the harderst part of automating is knowing the optimal way. We approach the problem by monitoring manual systems, where humans achieve success that we want to replicate. And we believe in experimentation, so having multiple environments and documenting their differences. If you want quick and cheap automation buy a power plug that has a timer, it's probably good enough.

The frontend code can be found here: [https://github.com/internet-of-plants/client](https://github.com/internet-of-plants/client)

The updates are automatic and happen over the air. Eventually the updates will demand signed binaries. Binary compression also is possible with gzip.

TODO: implement tooling that makes compression out of the box.

# Panics

If some irrecoverable error happens the device will halt and constantly try to update it's binary. All panics are reported to the network (if available). The owner of the device must then provide an OTA (Over The Air) update to fix it.

Since our system is very small and event driven an irrecoverable error probably won't be solved by restarting, since the error will just happen again almost immediately. Panics represent a bug in the code and shouldn't happen, but they are our last stand between us and UB, and a crash is much better than silent problems.

If there is no network available the device will halt forever and will need to be restarted/updated physically (through the serial port).

# Backends

The project has been designed and tested for the ESP8266-12F board (nodemcu).

Datasheet: https://www.electrodragon.com/w/ESP-12F_ESP8266_Wifi_Board

It also has a desktop driver which is used mostly to debug and test high level components. But we intend to develop drivers for other embed systems and improve the desktop one.

# Dependencies

PlatformIO (nodemcu2 + arduino framework)

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

# Build

You can just deploy the code to the nodemcu using PlatformIO. Choose `env:release` to deploy to production. After the first deploy you can use the update server.

To do that go to the frontend, find the appropriate plant and provide the binary, in a few minutes it should start the upload to the hardware. You will be able to keep track of the version running through the logs or last events.

# TODO

Grep for "TODO"'s to find the known missing pieces