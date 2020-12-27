# Internet of Plants - Embedded Software

Datasheet: https://www.electrodragon.com/w/ESP-12F_ESP8266_Wifi_Board

# Dependencies

PlatformIO (nodemcu + arduino framework)

Needs OpenSSL in PATH

On linux you have to handle configs and permissions to deploy to serial port

```
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo tee /etc/udev/rules.d/99-platformio-udev.rules
sudo service udev restart
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
```

# Build

After cloning you must set the configurations. To do that run this command:

`cp include/configuration.h.example include/configuration.h`

And fill all the constants with the appropriate value

Then you can deploy the code to the nodemcu using platform-io.

# TODO

Grep for "TODO"'s to find the known missing pieces
