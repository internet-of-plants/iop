### Public Methods

**DHT.h**

Repository: https://github.com/adafruit/DHT-sensor-library

Examples: https://github.com/adafruit/DHT-sensor-library/tree/master/examples

More information at: https://playground.arduino.cc/Main/DHTLib

```
// Define DHT_DEBUG to log to Serial
#define DHT_DEBUG

// Each sensor type has a identification number
#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

class DHT {
public:
	// The count param is currently ignored since the algorithm adapts itself dynamically
	// Type is defined according to the DHT11/DHT21/DHT22/AM2301 macros
    DHT(uint8_t pin, uint8_t type, uint8_t count=6);

    // Initialize sensor
    void begin(void);

    // If S is True it will return Fahrenheit otherwise Celsius
    // Devices provide Celsius data by default
    //
    // The hardware needs a interval of a minimum of 2 seconds between measurements (for DHT11 it's 1 second)
    // The cache enforces the 2 seconds limit, to override it and force the read set 'force = true'
	//
	// Returns NaN if the sensor's type is not valid (DHT11/DHT21/DHT22/AM2301)
    float readTemperature(bool S=false, bool force=false);

    // Perform convertion between Celsius and Fahrenheit data
    float convertCtoF(float);
    float convertFtoC(float);

    // Heat index is the how the current temperature is experienced (apparent temperature)
    // Uses fahrenheit for calculations (it will convert Celsius before executing the algo)
    //
    // Using both Rothfusz and Steadman's equations
    // (Rothfusz is better for heat indexes above 80 Steadman's is for the rest)
    // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
    float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit=true);

    // Reads humidity in percentage
    //
	// The force param is currently ignored (always obey cache)
    float readHumidity(bool force=false);

    // Read temperature and humidity from sensor
    // Returns cached value if last measurement was made less than 2 seconds ago
    //
    // The hardware needs a interval of a minimum of 2 seconds between measurements (for DHT11 it's 1 second)
    // The cache enforces the 2 seconds limit, to override it and force the read set 'force = true'
    boolean read(bool force=false);
}

class InterruptLock {
    // Locks interrupt until destructor is called
    InterruptLock();
    // Unlock interrupt
    ~InterruptLock();
}
```

**DHT_U.h**

Unifies DHT with Adafruit_Sensor.h library (returning its data-structures)
- sensor_t
- sensors_event_t

Repository: https://github.com/adafruit/DHT-sensor-library

Repository: https://github.com/adafruit/Adafruit_Sensor

Examples: https://github.com/adafruit/DHT-sensor-library/tree/master/examples

More information at: https://playground.arduino.cc/Main/DHTLib

```
#define DHT_SENSOR_VERSION 1

class DHT_Unified {
public:
	// The count param is currently ignored since the algorithm adapts itself dynamically
	//
	// tempSensorId and humiditySensorId are passed to sensor_t's sensor_id retrieved when calling temperature() and humidity()
    DHT_Unified(uint8_t pin, uint8_t count = 6, int32_t tempSensorId = -1, int32 humiditySensorId = -1);

    void begin(void);

    Temperature temperature();
    Humidity humidity();
}

class Temperature {
public:
	// Id is passed to sensor_t's sensor_id
    Temperature(DHT_Unified *parent, int32_t id);

    // Clears previous event and fill with data read from sensor
    // Defaults everything to 0 if sensor is not identified
    bool getEvent(sensors_event_t *event);

    // Clears sensor definition and build new one based on parent
    // Defaults everything to 0 if sensor is not identified
    void getSensor(sensor_t *sensor);
}

class Humidity {
public:
	// Id is passed to sensor_t's sensor_id
    Humidity(DHT_Unified *parent, int32_t id);

    // Clears previous event and fill with data read from sensor
    // Defaults everything to 0 if sensor is not identified
    bool getEvent(sensors_event_t *event);

    // Clears sensor definition and build new one based on parent
    // Defaults everything to 0 if sensor is not identified
    void getSensor(sensor_t *sensor);
}
```
