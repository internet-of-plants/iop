#include <SPI.h>
#include <SD.h>

#include <DS1302.h>
#include <virtuabotixRTC.h>

#include <DHT.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define DHT_TYPE DHT11
#define DHT_PIN 2

#define SOIL_TEMPERATURE_PIN 3

#define SOIL_RESISTIVITY_OUT_PIN 5
#define SOIL_RESISTIVITY_IN_PIN A4

#define LIGHT_PIN A3

#define SD_PIN 4

#define VOLTAGE 5.
#define ANALOG_READ_MAX 1024
#define ANALOG_WRITE_MAX 255
#define LOG_FILE "plant.log"
#define ERROR_FILE "error.log"

#define CLOCK_SCLK_PIN 6
#define CLOCK_IO_PIN 7
#define CLOCK_CE_PIN 8
virtuabotixRTC clock(CLOCK_SCLK_PIN, CLOCK_IO_PIN, CLOCK_CE_PIN);

typedef uint8_t Celsius;
typedef uint16_t Percentage;
typedef uint16_t AnalogRead;

struct State {
    Celsius air_temperature;
    Percentage air_humidity;
    Celsius air_heat_index;

    Celsius soil_temperature;
    AnalogRead soil_resistivity;

    AnalogRead light;
};

DHT dht(DHT_PIN, DHT_TYPE);
OneWire one_wire(SOIL_TEMPERATURE_PIN);
DallasTemperature soil_temperature(&one_wire);

struct State state = State { 0, 0, 0, 0, 0, 0 };

File file;

void setup() {
    Serial.begin(9600);

    // Needed for Leonardo
    while (!Serial) {}

    soil_temperature.begin();
    dht.begin();

    pinMode(SOIL_RESISTIVITY_OUT_PIN, OUTPUT);

    SD.begin(SD_PIN);

    //clock.setDS1302Time(10, 15, 0, 5, 28, 4, 2018);

    file = SD.open(LOG_FILE, FILE_WRITE);
}

void write_state_csv(Stream *fd) {
    fd->print(state.air_humidity);
    fd->print(",");
    fd->print(state.air_temperature);
    fd->print(",");
    fd->print(state.air_heat_index);
    fd->print(",");
    fd->print(state.soil_resistivity);
    fd->print(",");
    fd->print(state.soil_temperature);
    fd->print(",");
    fd->print(state.light);
    fd->print(",");

    write_date_csv(fd);

    fd->flush();
}

void read_light() {
    state.light = analogRead(LIGHT_PIN);
    // state.light = pow(((((150 * VOLTAGE)/(state.light*(VOLTAGE/ANALOG_READ_MAX))) - 150) / 70000),-1.25);
}

void read_soil_resistivity() {
    digitalWrite(SOIL_RESISTIVITY_OUT_PIN, HIGH);
    state.soil_resistivity = analogRead(SOIL_RESISTIVITY_IN_PIN);
}

void read_air_humidity_temperature() {
    state.air_humidity = dht.readHumidity();
    state.air_temperature = dht.readTemperature();

    if (isnan(state.air_humidity) || isnan(state.air_temperature)) {
        Serial.println("Failed to read from DHT sensor");
        return;
    }

    state.air_heat_index = dht.computeHeatIndex(state.air_temperature,
                                                state.air_humidity,
                                                /* isFahrenheit = */ false);
}

void read_soil_temperature() {
    soil_temperature.requestTemperatures();
    state.soil_temperature = soil_temperature.getTempCByIndex(0);
}

void write_date_csv(Stream *fd){
    clock.updateTime();
    fd->print(clock.seconds);
    fd->print(",");
    fd->print(clock.minutes);
    fd->print(",");
    fd->print(clock.hours);
    fd->print(",");
    fd->print(clock.dayofweek);
    fd->print(",");
    fd->print(clock.dayofmonth);
    fd->print(",");
    fd->print(clock.month);
    fd->print(",");
    fd->println(clock.year);

    fd->flush();
}

void loop() {
    read_light();
    read_air_humidity_temperature();
    read_soil_resistivity();
    read_soil_temperature();

    write_state_csv(&file);
    write_state_csv(&Serial);

    delay(2000);
}
