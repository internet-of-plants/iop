### Public Methods

**DallasTemperature.h**

Repository: https://github.com/milesburton/Arduino-Temperature-Control-Library/

Examples: https://github.com/milesburton/Arduino-Temperature-Control-Library/tree/master/examples

More information at: https://playground.arduino.cc/Learning/OneWire

```
// OneWire commands

// false disables all accesses to ALARM
// Makes 16 more bits available per device
// DS18S20 and DS1820 don't have alarms
#define REQUIRESALARM true

// true enables heap allocations (new and delete operators are enabled)
#define REQUIRESNEW false

// Model IDs
#define DS18S20MODEL 0x10  // also DS1820
#define DS18B20MODEL 0x28
#define DS1822MODEL  0x22
#define DS1825MODEL  0x3B
#define DS28EA00MODEL 0x42

// Error Codes
#define DEVICE_DISCONNECTED_C -127
#define DEVICE_DISCONNECTED_F -196.6
#define DEVICE_DISCONNECTED_RAW -7040

class DallasTemperature {
public:
    void DallasTemperature(OneWire* oneWire);

    void DallasTemperature(void);
    void setOneWire(OneWire* oneWire);

    // Initialize bus
    void begin(void);

    // Return number of devices using same bus
    uint8_t getDeviceCount(void);

    // Return number of devices of the family DS18xxx using same bus
    uint8_t getDS18Count(void);

    // Returns true if address is valid
    bool validAddress(const uint8_t* deviceAddress);

    // Returns true if address is of the family of sensors the lib supports.
    bool validFamily(const uint8_t* deviceAddress);

    // Returns the address of a device by index
    bool getAddress(uint8_t* deviceAddress, uint8_t index);

    // Try to determine if device selected by address is connected to the bus
    bool isConnected(const uint8_t* deviceAddress);

    // Try to determine if device selected by address is connected to the bus and is enabling stratchpad update
    bool isConnected(const uint8_t* deviceAddress, uint8_t* scratchPad);

    // Read device's scratchpad (measured temperature, alarm configuration, resolution configuration and CRC - corruption protection)
    bool readScratchPad(const uint8_t* deviceAddress, uint8_t* scratchPad);

    // Write to device's scratchpad (measured temperature, alarm configuration, resolution configuration and CRC - corruption protection)
    void writeScratchPad(const uint8_t* deviceAddress, const uint8_t* scratchPad);

    // Read device's power requirements (returns true if a sensor is in parasite mode)
    bool readPowerSupply(const uint8_t* deviceAddress);

    // Get global conversion resolution (9, 10, 11 or 12 bits) taking every sensor in the bus into account (some sensors only support 12-bit resolution)
    uint8_t getResolution(void);

    // Set global conversion resolution to 9, 10, 11 or 12 bits
    void setResolution(uint8_t resolution);

    // Returns device's conversion resolution (9, 10, 11 or 12 bits)
    uint8_t getResolution(const uint8_t* deviceAddress);

    // Set device's conversion resolution to 9, 10, 11 or 12 bits
    bool setResolution(const uint8_t* deviceAddress, uint8_t resolution, bool skipGlobalBitResolutionCalculation = false);

    // If wait is set it will busy block while measuring, else uses async conversion)
    void setWaitForConversion(bool waitForConversion);
    bool getWaitForConversion(void);

    // If check is set it will poll non-parasite sensors to check if conversion was completed, else it will busy block for the maximum ammount of time conversion may take (not exactly ideal)
    void setCheckForConversion(bool checkForConversion);
    bool getCheckForConversion(void);

    // Start temperature conversion in every sensors (by default blocks until conversion ends)
    void requestTemperatures(void);

    // Start temperature conversion by address (by default blocks until conversion ends)
    bool requestTemperaturesByAddress(const uint8_t* deviceAddress);

    // Start temperature conversion by sensor index - slows and not recommended - (by default blocks until conversion ends)
    bool requestTemperaturesByIndex(uint8_t index);

    // Returns temperature raw value (12 bit integer of 1/128 degrees C)
    int16_t getTemp(const uint8_t* deviceAddress);

    // Returns temperature in degrees C
    float getTempC(const uint8_t* deviceAddress);

    // Returns temperature in degrees F
    float getTempF(const uint8_t* deviceAddress);

    // Get temperature for device index (slow and not recommended)
    float getTempCByIndex(uint8_t index);

    // Get temperature for device index (slow and not recommended)
    float getTempFByIndex(uint8_t index);

    // Returns true if at least one sensor in the bus requires parasite power
    bool isParasitePowerMode(void);

    // Ask first sensor in the wire if conversion is completed (problematic if other sensors weren't finished but are read anyway?)
    bool isConversionComplete(void);

    // Each conversion resolution bits (9, 10, 11 and 12 bits) has a maximum time of conversion
    // You can call delay(millisToWaitForConversion(bitResolution)) for busy blocking (parasites can't be polled, so you have to wait)
    int16_t millisToWaitForConversion(uint8_t resolution);

    // Sets the high alarm temperature for a device (-55*C to 125*C)
    void setHighAlarmTemp(const uint8_t* deviceAddress, int8_t temperature);

    // Sets the low alarm temperature for a device (-55*C to 125*C)
    void setLowAlarmTemp(const uint8_t* deviceAddress, int8_t temperature);

    // Returns the high alarm temperature for a device (-55*C to 125*C)
    int8_t getHighAlarmTemp(const uint8_t* deviceAddress);

    // Returns the low alarm temperature for a device (-55*C to 125*C)
    int8_t getLowAlarmTemp(const uint8_t* deviceAddress);

    // Resets internal variables used for the alarm search (internal)
    void resetAlarmSearch(void);

    // Search the bus for devices with active alarms (internal)
    bool alarmSearch(uint8_t* deviceAddress);

    // Returns true if a specific device has an alarm
    bool hasAlarm(const uint8_t* deviceAddress);

    // Returns true if any device is reporting an alarm on the bus
    bool hasAlarm(void);

    // Calls alarm handler for all devices with an active alarm
    void processAlarms(void);

    // Sets function as alarm handler `void AlarmHandler(const uint8_t* deviceAddress)`
    void setAlarmHandler(const AlarmHandler *);

    // Returns true if an AlarmHandler has been set
    bool hasAlarmHandler(void);

    // If no alarm is set there will be 16 bits available of memory in each sensor
    void setUserData(const uint8_t* deviceAddress, int16_t data);
    void setUserDataByIndex(uint8_t index, int16_t data);
    int16_t getUserData(const uint8_t* deviceAddress);
    int16_t getUserDataByIndex(uint8_t index);

    // Convert from Celsius to Fahrenheit
    static float toFahrenheit(float celsius);

    // Convert from Fahrenheit to Celsius
    static float toCelsius(float fahrenheit);

    // Convert from raw to Celsius
    static float rawToCelsius(int16_t raw);

    // Convert from raw to Fahrenheit
    static float rawToFahrenheit(int16_t raw);

    // Initialize memory area
    void* operator new (unsigned int size);

    // Delete memory reference
    void operator delete(void* object);
}
```

**OneWire.h**

Repository: https://github.com/PaulStoffregen/OneWire/

More information at: https://playground.arduino.cc/Learning/OneWire

```
// 0 disables public methods to search for devices in OneWire bus
ONEWIRE_SEARCH 1

// 0 disables CRC (data integrity test) checks completely
ONEWIRE_CRC 1

// 0 uses compact but slow algorithm (decreases code size by 250 bytes)
ONEWIRE_CRC8_TABLE 1

// 0 disables 16 bits CRC tests
ONEWIRE_CRC16 1

class OneWire {
public:
    OneWire(uint8_t pin);

    // Initializes bus and reset search (if enabled)
    void begin(uint8_t pin);

    // OneWire reset cycle, 1 if there is a device
    // 0 if there are no devices/bus is shorted/held low for more than 250ms/something horrible happened
    uint8_t reset(void);
    
    // Select a ROM in bus, you must call reset() first
    void select(const uint8_t rom[8]);

    // Do a ROM skip in every device in bus
    void skip(void);

    // Write byte to device, set power to 1 if the device is parasitically powered
    // (you must call depower() or write with power = 0 later to depower it)
    // DS18S20 in parasite mode is an example
    // With power = 0 the pin you go tri-state at the end of the write to avoid heating in a short "or other mishap"
    //
    // Calls write_bit internally
    void write(uint8_t v, uint8_t power = 0);

    // Iter through buffer calling write with its data
    // it's your responsibility to ensure the buffer is big enough to hold the data
    void write_bytes(const uint8_t *buf, uint16_t count, bool power = 0);

    // Read byte from bus
    uint8_t read(void);

    // Read sequence of bytes to buffer
    // it's your responsibility to ensure the buffer is big enough
    void read_bytes(uint8_t *buf, uint16_t count);

    // Write bit to bus (leaves bus powered after)
    void write_bit(uint8_t v);

    // Read a bit. Port and bit is used to cut lookup time and provide more certain timing.    
    uint8_t read_bit(void);

    // Stops powering the bus
    // You only need to call after write with power = 1 or calling write_bit() without calling another write_bit
    // It's good practice to depower the bus when not needed to (just in case the bus is shorted)
    void depower(void);

#if ONEWIRE_SEARCH
    // Clears the search state (to start another one from the beggining)
    void reset_search();

    // Set the search state to find a device with the specified family_code
    // on the next call to search(*newAddr)
    void target_search(uint8_t family_code);

    // Asks bus for next device in it, returns 1 if there is one
    // 0 if there are no devices/no devices left/bus is shorted/something horrible happened
    // The address can be retrieved by accessing the OneWire::address variable or *newAddr
    //
    // Keeps a state, to retrieve the next device in bus, to clear call reset_search()
    // Checking the CRC to make sure you didn't get garbage is recommended
    // The order will always be the same for the same setup
    bool search(uint8_t *newAddr, bool search_mode = true);
#endif

#if ONEWIRE_CRC
    // Computes a Dallas Semiconductor 8 bit CRC (the CRC shows up in the ROM and the registers)
    // if ONEWIRE_CRC8_TABLE is 1 a 2x16 lookup table is used
    // if not the CRC is computed directly (which is way slower, but a little smaller)
    uint8_t crc8(const uint8_t *addr, uint8_t len);

#if ONEWIRE_CRC16
    // Computes Dallas Semiconductor 16 bit CRC and checks against CRC provided by the OneWire network
    //
    // The network provides the CRC inverted
    // A starting CRC value may be provided (crc parameter)
    bool check_crc16(const uint8_t *input, uint16_t len, const uint8_t *inverted_crc, uint16_t crc = 0);

    // Computes Dallas Semiconductor 16 bit CRC, needed to check the
    // integrity of data received from many OneWire devices
    // This CRC is not the same CRC as retrieved from the OneWire network
    //   - The CRC is transmitted bitwise inverted
    //   - The binary representation may vary depending on the endianness of the processor
    // A starting CRC value may be provided (crc parameter)
    uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);        
#endif
}

#endif
```
