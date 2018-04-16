#include <SPI.h>
#include <SD.h>

#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHT_PIN 2

#define SOIL_TEMPERATURE_PIN 3

#define SOIL_RESISTIVITY_OUT_PIN 4
#define SOIL_RESISTIVITY_IN_PIN A4

#define LIGHT_PIN A5

#define DHT_TYPE DHT11

#define VOLTAGE 5.
#define ANALOG_READ_MAX 1024
#define ANALOG_WRITE_MAX 255

struct RawState {
	uint16_t soil_resistivity;
	uint16_t light;
};

struct Environment {
	float humidity;
	float temperature_c;
	float heat_index;
};

struct State {
	Environment air;
	Environment soil;
	float light;
	RawState raw;
};

DHT dht(DHT_PIN, DHT_TYPE);
OneWire one_wire(SOIL_TEMPERATURE_PIN);
DallasTemperature soil_temperature(&one_wire);

struct State state = State {
	Environment { 0, 0, 0 },
	Environment { 0, 0, 0 },
	0,
	RawState { 0, 0 }
};

File file;

void setup() {
	Serial.begin(9600);
	while (!Serial) {}

	soil_temperature.begin();
	dht.begin();

	pinMode(SOIL_RESISTIVITY_OUT_PIN, OUTPUT);
	digitalWrite(SOIL_RESISTIVITY_OUT_PIN, HIGH);

	if (!SD.begin(4)) {
		Serial.println("SD initialization failed!");
 		goto error;
	}
	
	file = SD.open("plant.log", FILE_WRITE);
	if (!file) {
		Serial.println("error opening plant.log");
		goto error;
	}

	return;
error:
	while (1) {}
}

void print_state() {
	while (file.available()) {
		Serial.write(file.read());
	}
}

void save_state(Stream *fd) {
	fd->println("Ar");
	fd->print("Umidade:          ");
	fd->print(state.air.humidity);
	fd->println("%");
	fd->print("Temperatura:      ");
	fd->print(state.air.temperature_c);
	fd->println(" *C");
	fd->print("Indice de Calor: ");
	fd->print(state.air.heat_index);
	fd->println(" *C");

	fd->println("Solo");
	fd->print("Resistividade do solo raw: ");
	fd->println(state.raw.soil_resistivity);
	fd->print("Temperatura do solo:       ");
	fd->print(state.soil.temperature_c);
	fd->println(" *C");

	fd->println("Luz:");
	fd->print(state.light);
	fd->println(" lux");
	fd->println("");
}

void read_light() {
	state.raw.light = analogRead(LIGHT_PIN);
	state.light = pow(((((150 * VOLTAGE)/(state.raw.light*(VOLTAGE/ANALOG_READ_MAX))) - 150) / 70000),-1.25);
}

void read_soil_resistivity() {
  state.raw.soil_resistivity = analogRead(SOIL_RESISTIVITY_IN_PIN);
}

void read_air_humidity_temperature() {
	state.air.humidity = dht.readHumidity();
	state.air.temperature_c = dht.readTemperature();

	if (isnan(state.air.humidity) || isnan(state.air.temperature_c)) {
		Serial.println("Failed to read from DHT sensor");
		return;
	}

	state.air.heat_index = dht.computeHeatIndex(state.air.temperature_c,
	                                           state.air.humidity,
	                                           /* isFahrenheit = */ false);
}

void read_soil_temperature() {
  soil_temperature.requestTemperatures();
  state.soil.temperature_c = soil_temperature.getTempCByIndex(0);
}

void step() {
	read_light();
	read_air_humidity_temperature();
	read_soil_resistivity();
	read_soil_temperature();
}

void loop() {
        Serial.println("asdasd");
	step();
	save_state(&file);
        save_state(&Serial);
	//print_state();
	delay(1000);
}
