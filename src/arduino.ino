#include <SPI.h>
#include <SD.h>

#include <DS1302.h>
#include <virtuabotixRTC.h>

#include <DHT.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define DHT_PIN 2
#define SOIL_TEMPERATURE_PIN 3
#define SOIL_RESISTIVITY_OUT_PIN 5
#define SOIL_RESISTIVITY_IN_PIN A4
#define LIGHT_PIN A3
#define DHT_TYPE DHT11
#define VOLTAGE 5.
#define ANALOG_READ_MAX 1024
#define ANALOG_WRITE_MAX 255
#define LOG_FILE "plant.log"

virtuabotixRTC myRTC(6,7,8);

int segundos = 10;
int minutos= 15;
int hora= 00;
int dia_semana = 5;
int dia_mes= 19;
int mes = 4;
int ano= 2018;
/*
 * patch 1.2 fim
*/

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
  // Needed for Leonardo
  while (!Serial) {}

  soil_temperature.begin();
  dht.begin();

  pinMode(SOIL_RESISTIVITY_OUT_PIN, OUTPUT);
  digitalWrite(SOIL_RESISTIVITY_OUT_PIN, HIGH);

  if (!SD.begin(4)) {
    Serial.println("Failed to initialize SD card");

/*
 * update 18/04/2018 - patch 1.3 inicio
 * declarando parametros inicias clock
*/
  myRTC.setDS1302Time(segundos, minutos, hora, dia_semana, dia_mes, mes, ano);
/*
 * patch 1.3 fim
*/
  }
}

void write_state_matrix(Stream *fd) {


  fd->print(state.air.humidity);
  fd->print(",");
  fd->print(state.air.temperature_c);
  fd->print(",");
  fd->print(state.air.heat_index);
  fd->print(",");
  fd->print(state.raw.soil_resistivity);
  fd->print(",");
  fd->print(state.soil.temperature_c);
  fd->print(",");
  fd->print(state.light);
  fd->print(",");
  
  fd->flush();
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

/*
 * update 18/04/2018 - patch 1.4 inicio
 * conversa com o clock
*/

void hora_data(Stream *hd){
  
  // le informacoes do ci
  myRTC.updateTime();
  hd->print(myRTC.seconds);
  hd->print(",");
  hd->print(myRTC.minutes);
  hd->print(",");
  hd->print(myRTC.hours);
  hd->print(",");
  hd->print(myRTC.dayofweek);
  hd->print(",");
  hd->print(myRTC.dayofmonth);
  hd->print(",");
  hd->print(myRTC.month);
  hd->print(",");
  hd->println(myRTC.year);
  }
/*
 * patch 1.4 fim
*/


void loop() {

/*
  Serial.println("Loop");
*/
  step();

  while (!(file = SD.open(LOG_FILE, FILE_WRITE))) {}
  write_state_matrix(&file);
  hora_data(&file);
  file.close();
  delay(1000);

  write_state_matrix(&Serial);
  hora_data(&Serial);
  delay(1000);
}
