#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#include <main.hpp>
#include <configuration.h>
#include <api.hpp>
#include <measurement.hpp>

OneWire oneWire(oneWireBus);
DallasTemperature soilTemperatureSensor(&oneWire);

// This is the sensor we are testing with, if you use another sensor you must change the DHT type
DHT dht(dhtPin, DHT22);

// Honestly global state is garbage, but it's basically loop()'s state
unsigned long lastTime = 0;
Option<String> authToken = Option<String>();

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  dht.begin();
  soilTemperatureSensor.begin();

  Serial.begin(9600);
  delay(100);
  Serial.println("Setup");

  #ifndef IOP_ONLINE
    authToken = Option<String>("4"); // chosen by fair dice roll, garanteed to be random
  #endif

  connect();
}

void loop() {
  const unsigned long currTime = millis();
  if (lastTime == 0 || lastTime + timerDelay < currTime) {
    digitalWrite(LED_BUILTIN, HIGH);
    lastTime = currTime;
    Serial.println("Timer triggered");
    Serial.flush();

    if (authToken.isNone() && (authToken = generateToken()).isNone()) {
      Serial.println("Unable to get new token");
      return;
    }

    const Event event = (Event) {
      .airTemperatureCelsius = measureAirTemperatureCelsius(dht),
      .airHumidityPercentage = measureAirHumidityPercentage(dht),
      .airHeatIndexCelsius = measureAirHeatIndexCelsius(dht),
      .soilResistivityRaw = 800, // TODO
      .soilTemperatureCelsius = measureSoilTemperatureCelsius(soilTemperatureSensor),
      .plantId = iopPlantId,
    };

    if (authToken.isSome()) {
      sendEvent(authToken.unwrap(), event);
    }
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    Serial.print("Sleep for ");
    // The if above makes sure this won't ever wrap
    Serial.println(lastTime + timerDelay - currTime);
    Serial.flush();
    delay(lastTime + timerDelay - currTime);
  }
}