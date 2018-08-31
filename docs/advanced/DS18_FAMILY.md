### Public Methods

**DallasTemperature.h**

Repository: https://github.com/milesburton/Arduino-Temperature-Control-Library/
Examples: https://github.com/milesburton/Arduino-Temperature-Control-Library/tree/master/examples
More information at: https://playground.arduino.cc/Learning/OneWire 

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

**TODO: OneWire.h**

Repository: https://github.com/PaulStoffregen/OneWire/
More information at: https://playground.arduino.cc/Learning/OneWire
