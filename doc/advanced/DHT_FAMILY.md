### Public Methods

**DHT.h**

Repository: https://github.com/adafruit/DHT-sensor-library
Examples: https://github.com/adafruit/DHT-sensor-library/tree/master/examples
More information at: https://playground.arduino.cc/Main/DHTLib

    DHT(uint8_t pin, uint8_t type, uint8_t count=6);

    // Initialize sensor
    void begin(void);

    // If S is True it will return Fahrenheit else Celsius
    float readTemperature(bool S=false, bool force=false);

    float convertCtoF(float);
    float convertFtoC(float);

    float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit=true);

    float readHumidity(bool force=false);

    // Read temperature and humidity from sensor
    // Returns cached value if last measurement was made less than 2 seconds ago
    boolean read(bool force=false);
