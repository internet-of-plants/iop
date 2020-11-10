#include <flash.hpp>
#include <api.hpp>
#include <reset.hpp>
#include <server.hpp>
#include <sensors.hpp>
#include <log.hpp>
#include <configuration.h>
#include <utils.hpp>

// TODO: https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/timeSync.cpp
// TODO: https://github.com/maakbaas/esp8266-iot-framework/blob/master/src/updater.cpp

class EventLoop {
private:
  Sensors sensors;
  // Prevents stack-overflow
  std::unique_ptr<Api> api;
  std::unique_ptr<CredentialsServer> credentialsServer;
  Log logger;
  Flash flash;

  unsigned long nextTime;
  unsigned long nextYieldLog;
  
public:
  void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    reset::setup();
    this->logger.setup();
    this->sensors.setup();
    this->flash.setup();
    this->api->setup();
  }

  void loop() noexcept {
    const InterruptEvent event = interruptEvent;
    interruptEvent = NONE;

    switch (event) {
      case NONE:
        break;
      case FACTORY_RESET:
        #ifdef IOP_FACTORY_RESET
        this->logger.info("Resetting all user information saved in flash storage");
         this->flash.removeWifiConfig();
         this->flash.removeAuthToken();
         this->flash.removePlantId();
         this->api->disconnect();
        #endif
        return;
      case ON_CONNECTION:
        // #ifdef IOP_ONLINE
        //   this->logger.info("WiFi connected (" + WiFi.localIP().toString() + "): " + String(wifi_station_get_connect_status()));
        //   struct station_config config = {0};
        //   wifi_station_get_config(&config);

        //   WiFi.mode(WIFI_STA);

        //   const auto maybeCurrConfig = this->flash.readWifiConfig();
        //   if (maybeCurrConfig.isSome()) {
        //     const struct station_config & currConfig = maybeCurrConfig.asRef().expect("Current network config missing when it shouldn't");
        //     const bool sameSsid = memcmp(currConfig.ssid, config.ssid, 32);
        //     const bool samePsk = memcmp(currConfig.password, config.password, 64);
        //     if (sameSsid && samePsk) {
        //       break;
        //     }
        //   }

        //   this->flash.writeWifiConfig(config);
        //   String msg = (char*) config.ssid;
        //   msg += ", " + String((char*) config.password);
        //   if (config.bssid_set) {
        //     msg += ", " + String((char*) config.bssid);
        //   }
        //   this->logger.info("Connected to: " + msg);
        // #endif
        return;
    }

    //const unsigned long now = millis();
    // auto authToken = this->flash.readAuthToken();
    // auto plantId = this->flash.readPlantId();

    // if (this->api->isConnected() && authToken.isSome() && plantId.isSome()) {
    //   this->credentialsServer.close();
    // }
    
    // if (!this->api->isConnected() || authToken.isNone()) {
    //   auto result = this->credentialsServer.serve(this->flash.readWifiConfig(), std::move(authToken));
    //   if (result.isOk()) {
    //     auto opt = result.expectOk("Result is err but shouldn't: at loop()");
    //     if (opt.isSome()) {
    //       this->flash.writeAuthToken(opt.expect("AuthToken is None but shouldn't: at loop()"));
    //     }
    //   } else if (result.isErr()) {
    //     switch (result.expectErr("Result is ok but shouldn't: at loop()")) {
    //       case ServeError::REMOVE_WIFI_CONFIG:
    //         this->flash.removeWifiConfig();
    //         break;
    //     }
    //   } else {
    //     panic_("Invalid result state");
    //   }
    // } else if (plantId.isNone()) {
    //   const auto authToken_ = authToken.expect("authToken is None at loop");
    //   const String token = (char*) authToken_.data();
    //   auto maybePlantId = this->api->registerPlant(token);
    //   if (maybePlantId.isErr()) {
    //     logger.error("Unable to get plant id");
    //     auto maybeStatusCode = maybePlantId.expectErr("maybePlantId is Ok but shouldn't");
    //     if (maybeStatusCode.isSome() && maybeStatusCode.expect("maybeStatusCode is None") == 403) {
    //       this->flash.removeAuthToken();
    //     }
    //   } else if (maybePlantId.isOk()) {
    //     const auto plantId = maybePlantId.expectOk("maybePlantId is Err but shouldn't");
    //     this->flash.writePlantId(plantId);
    //   } else {
    //     panic_("maybePlantId was empty when it shouldn't");
    //   }
    // } else if (nextTime <= now) {
    //   nextTime = now + interval;
    //   this->logger.info("Timer triggered");
      
    //   digitalWrite(LED_BUILTIN, HIGH);
    //   const auto token = authToken.expect("Calling sendEvent, authToken is None");
    //   const auto id = plantId.expect("Calling Sensors::measure, plantId is None");
      //auto maybeStatus = api.registerEvent(token, sensors.measure(id));
      auto maybeStatus = this->api->registerEvent({0}, {0});

      // if (maybeStatus.isNone()) {
      //   return;
      // }
      // const auto status = maybeStatus.expect("Status is none");
      // if (status == 403) {
      //   logger.warn("Auth token was refused, deleting it");
      //   this->flash.removeAuthToken();
      // } else if (status == 404) {
      //   logger.warn("Plant Id was not found, deleting it");
      //   this->flash.removePlantId();
      // }
      // digitalWrite(LED_BUILTIN, LOW);
    // } else if (nextYieldLog <= now) {    
    //   nextYieldLog = now + 10000;
    //   logger.debug("Waiting");
    // }
  }

  void operator=(EventLoop & other) = delete;
  void operator=(EventLoop && other) {
    this->sensors = std::move(other.sensors);
    this->api = std::move(other.api);
    this->logger = std::move(other.logger);
    this->credentialsServer = std::move(other.credentialsServer);
    this->flash = std::move(other.flash);
  }
  EventLoop(const String host):
    sensors(soilResistivityPowerPin, soilTemperaturePin, airTempAndHumidityPin, dhtVersion),
    api(new Api(host, logLevel)),
    credentialsServer(new CredentialsServer(host, logLevel)),
    logger(logLevel, "LOOP"),
    flash(logLevel) {}
};

EventLoop eventLoop(host.asRef().expect("Host cant be none"));

void setup() {
  eventLoop.setup();
}

void loop() {
  eventLoop.loop();
  yield();
}