### Public Methods

**DallasTemperature.h**

Repository: https://github.com/milesburton/Arduino-Temperature-Control-Library/
More information at: 

    void DallasTemperature(OneWire*);

    void DallasTemperature();
    void setOneWire(OneWire*);

    // Initialize bus
    void begin(void);

    // Return number of devices using same bus
    uint8_t getDeviceCount(void);

    // Return number of devices of the family DS18xxx using same bus
    uint8_t getDS18Count(void);

    // Returns true if address is valid
    bool validAddress(const uint8_t*);

    // Returns true if address is of the family of sensors the lib supports.
    bool validFamily(const uint8_t* deviceAddress);

    // Returns the address of a device by index
    bool getAddress(uint8_t*, uint8_t);

    // Try to determine if device selected by address is connected to the bus
    bool isConnected(const uint8_t*);

    // Try to determine if device selected by address is connected to the bus and is enabling stratchpad update
    bool isConnected(const uint8_t*, uint8_t*);

    // Read device's scratchpad (measured temperature, alarm configuration, resolution configuration and CRC - corruption protection)
    bool readScratchPad(const uint8_t*, uint8_t*);

    // Write to device's scratchpad (measured temperature, alarm configuration, resolution configuration and CRC - corruption protection)
    void writeScratchPad(const uint8_t*, const uint8_t*);

    // Read device's power requirements (returns true if a sensor is in parasite mode)
    bool readPowerSupply(const uint8_t*);

    // Get global conversion resolution (9, 10, 11 or 12 bits) taking every sensor in the bus into account (some sensors only support 12-bit resolution)
    uint8_t getResolution();

    // Set global conversion resolution to 9, 10, 11 or 12 bits
    void setResolution(uint8_t);

    // Returns device's conversion resolution (9, 10, 11 or 12 bits)
    uint8_t getResolution(const uint8_t*);

    // Set device's conversion resolution to 9, 10, 11 or 12 bits
    bool setResolution(const uint8_t*, uint8_t, bool skipGlobalBitResolutionCalculation = false);

    // If wait is set it will busy block while measuring, else uses async conversion)
    void setWaitForConversion(bool);
    bool getWaitForConversion(void);

    // If check is set it will poll non-parasite sensors to check if conversion was completed, else it will busy block for the maximum ammount of time conversion may take (not exactly ideal)
    void setCheckForConversion(bool);
    bool getCheckForConversion(void);

    // Start temperature conversion in every sensors (by default blocks until conversion ends)
    void requestTemperatures(void);

    // Start temperature conversion by address (by default blocks until conversion ends)
    bool requestTemperaturesByAddress(const uint8_t*);

    // Start temperature conversion by sensor index - slows and not recommended - (by default blocks until conversion ends)
    bool requestTemperaturesByIndex(uint8_t);

    // Returns temperature raw value (12 bit integer of 1/128 degrees C)
    int16_t getTemp(const uint8_t*);

    // Returns temperature in degrees C
    float getTempC(const uint8_t*);

    // Returns temperature in degrees F
    float getTempF(const uint8_t*);

    // Get temperature for device index (slow and not recommended)
    float getTempCByIndex(uint8_t);

    // Get temperature for device index (slow and not recommended)
    float getTempFByIndex(uint8_t);

    // Returns true if at least one sensor in the bus requires parasite power
    bool isParasitePowerMode(void);

    // Ask first sensor in the wire if conversion is completed (problematic if other sensors weren't finished but are read anyway?)
    bool isConversionComplete(void);

    // Each conversion resolution bits (9, 10, 11 and 12 bits) has a maximum time of conversion 
    // You can call delay(millisToWaitForConversion(bitResolution)) for busy blocking (parasites can't be polled, so you have to wait)
    int16_t millisToWaitForConversion(uint8_t);

**OneWire.h**

Repository: https://github.com/PaulStoffregen/OneWire/
More information at: https://playground.arduino.cc/Learning/OneWire

    OneWire(uint8_t pin);
    void begin(uint8_t pin);

    // Perform a 1-Wire reset cycle. Returns 1 if a device responds
    // with a presence pulse.  Returns 0 if there is no device or the
    // bus is shorted or otherwise held low for more than 250uS
    uint8_t reset(void);

    // Issue a 1-Wire rom select command, you do the reset first.
    void select(const uint8_t rom[8]);

    // Issue a 1-Wire rom skip command, to address all on bus.
    void skip(void);

    // Write a byte. If 'power' is one then the wire is held high at
    // the end for parasitically powered devices. You are responsible
    // for eventually depowering it by calling depower() or doing
    // another read or write.
    void write(uint8_t v, uint8_t power = 0);

    void write_bytes(const uint8_t *buf, uint16_t count, bool power = 0);

    // Read a byte.
    uint8_t read(void);

    void read_bytes(uint8_t *buf, uint16_t count);

    // Write a bit. The bus is always left powered at the end, see
    // note in write() about that.
    void write_bit(uint8_t v);

    // Read a bit.
    uint8_t read_bit(void);

    // Stop forcing power onto the bus. You only need to do this if
    // you used the 'power' flag to write() or used a write_bit() call
    // and aren't about to do another read or write. You would rather
    // not leave this powered if you don't have to, just in case
    // someone shorts your bus.
    void depower(void);

    // Clear the search state so that if will start from the beginning again.
    void reset_search();

    // Setup the search to find the device type 'family_code' on the next call
    // to search(*newAddr) if it is present.
    void target_search(uint8_t family_code);

    // Look for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are
    // no devices, or you have already retrieved all of them.  It
    // might be a good idea to check the CRC to make sure you didn't
    // get garbage.  The order is deterministic. You will always get
    // the same devices in the same order.
    bool search(uint8_t *newAddr, bool search_mode = true);

    // Compute a Dallas Semiconductor 8 bit CRC, these are used in the
    // ROM and scratchpad registers.
    static uint8_t crc8(const uint8_t *addr, uint8_t len);

    // Compute the 1-Wire CRC16 and compare it against the received CRC.
    // Example usage (reading a DS2408):
    //    // Put everything in a buffer so we can compute the CRC easily.
    //    uint8_t buf[13];
    //    buf[0] = 0xF0;    // Read PIO Registers
    //    buf[1] = 0x88;    // LSB address
    //    buf[2] = 0x00;    // MSB address
    //    WriteBytes(net, buf, 3);    // Write 3 cmd bytes
    //    ReadBytes(net, buf+3, 10);  // Read 6 data bytes, 2 0xFF, 2 CRC16
    //    if (!CheckCRC16(buf, 11, &buf[11])) {
    //        // Handle error.
    //    }     
    //          
    // @param input - Array of bytes to checksum.
    // @param len - How many bytes to use.
    // @param inverted_crc - The two CRC16 bytes in the received data.
    //                       This should just point into the received data,
    //                       *not* at a 16-bit integer.
    // @param crc - The crc starting value (optional)
    // @return True, iff the CRC matches.
    static bool check_crc16(const uint8_t* input, uint16_t len, const uint8_t* inverted_crc, uint16_t crc = 0);

    // Compute a Dallas Semiconductor 16 bit CRC.  This is required to check
    // the integrity of data received from many 1-Wire devices.  Note that the
    // CRC computed here is *not* what you'll get from the 1-Wire network,
    // for two reasons:
    //   1) The CRC is transmitted bitwise inverted.
    //   2) Depending on the endian-ness of your processor, the binary
    //      representation of the two-byte return value may have a different
    //      byte order than the two bytes you get from 1-Wire.
    // @param input - Array of bytes to checksum.
    // @param len - How many bytes to use.
    // @param crc - The crc starting value (optional)
    // @return The CRC16, as defined by Dallas Semiconductor.
    static uint16_t crc16(const uint8_t* input, uint16_t len, uint16_t crc = 0);
