#include "api.hpp"
#include "fixed_string.hpp"
#include "utils.hpp"

#ifndef IOP_API_DISABLED
#include "ESP8266httpUpdate.h"

void Api::setup() const noexcept { this->network.setup(); }

bool Api::isConnected() const noexcept { return this->network.isConnected(); }
String Api::macAddress() const noexcept { return this->network.macAddress(); }
void Api::disconnect() const noexcept { this->network.disconnect(); }
LogLevel Api::loggerLevel() const noexcept { return this->logger.level(); }

// TODO: should we panic if Api::makeJson fails because of overflow? Or just
// report it? Is returning 400 enough?

Option<HttpCode> Api::reportPanic(const AuthToken &authToken,
                                  const Option<PlantId> &id,
                                  const PanicData &event) const noexcept {
  this->logger.debugln(F("Report panic:"), event.msg);

  const auto make = [authToken, &id, event](JsonDocument &doc) {
    if (id.isSome()) {
      doc["plant_id"] = UNWRAP_REF(id).asString().get();
    } else {
      doc["plant_id"] = nullptr;
    }
    doc["file"] = event.file.get();
    doc["line"] = event.line;
    doc["func"] = event.func.get();
    doc["msg"] = event.msg.get();
  };
  const auto maybeJson = this->makeJson<2048>(F("Api::reportPanic"), make);

  const auto token = authToken.asString();
  if (maybeJson.isNone()) {
    return Option<HttpCode>(400);
  }

  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp = this->network.httpPost(token, F("/panic"), *json);

#ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    return Option<HttpCode>();
  }

  return Option<HttpCode>(UNWRAP_REF(maybeResp).code);
#else
  return Option<HttpCode>(200);
#endif
}

Option<HttpCode> Api::registerEvent(const AuthToken &authToken,
                                    const Event &event) const noexcept {
  this->logger.debugln(F("Send event: "), event.plantId.asString());

  const auto make = [&event](JsonDocument &doc) {
    doc["air_temperature_celsius"] = event.storage.airTemperatureCelsius;
    doc["air_humidity_percentage"] = event.storage.airHumidityPercentage;
    doc["air_heat_index_celsius"] = event.storage.airHeatIndexCelsius;
    doc["soil_temperature_celsius"] = event.storage.soilTemperatureCelsius;
    doc["soil_resistivity_raw"] = event.storage.soilResistivityRaw;
    doc["firmware_hash"] = event.firmwareHash.asString().get();
    doc["plant_id"] = event.plantId.asString().get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::registerEvent"), make);
  if (maybeJson.isNone()) {
    // TODO: this endpoint also is used to detect updates
    // So we should make a request to look for new updates
    // Or the device will be stuck forever with broken code
    return Option<HttpCode>(400);
  }

  const auto &json = UNWRAP_REF(maybeJson);
  const auto token = authToken.asString();

  const auto maybeResp = this->network.httpPost(token, F("/event"), *json);

#ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    return Option<HttpCode>();
  }

  return Option<HttpCode>(UNWRAP_REF(maybeResp).code);
#else
  return Option<HttpCode>(200);
#endif
}

Result<AuthToken, Option<HttpCode>>
Api::authenticate(const StringView username,
                  const StringView password) const noexcept {
  this->logger.debugln(F("Authenticate IoP user: "), username);

  if (username.isEmpty() || password.isEmpty()) {
    this->logger.debugln(F("Empty username or password, at Api::authenticate"));
    return Option<HttpCode>(404);
  }

  const auto make = [username, password](JsonDocument &doc) {
    doc["email"] = username.get();
    doc["password"] = password.get();
  };
  const auto maybeJson = this->makeJson<256>(F("Api::authenticate"), make);
  if (maybeJson.isNone()) {
    return Option<HttpCode>(400);
  }

  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp = this->network.httpPost(F("/user/login"), *json);

#ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    return Option<HttpCode>();
  } else {
    const auto &resp = UNWRAP_REF(maybeResp);
    if (resp.code != 200) {
      return Option<HttpCode>(resp.code);
    }

    auto result = AuthToken::fromString(resp.payload);

    if (IS_ERR(result)) {
      switch (UNWRAP_ERR(result)) {
      case TOO_BIG:
        const auto lengthStr = std::to_string(resp.payload.length());
        this->logger.errorln(F("Auth token is too big: size = "), lengthStr);
        break;
      }
      return Option<HttpCode>(500);
    }
    return UNWRAP_OK(result);
  }
#else
  return AuthToken::empty();
#endif
}

Option<HttpCode> Api::reportError(const AuthToken &authToken, const PlantId &id,
                                  const StringView error) const noexcept {
  this->logger.debugln(F("Report error: "), error);

  const auto make = [id, error](JsonDocument &doc) {
    doc["plant_id"] = id.asString().get();
    doc["error"] = error.get();
  };
  const auto token = authToken.asString();
  const auto maybeJson = this->makeJson<300>(F("Api::reportError"), make);
  if (maybeJson.isNone()) {
    return Option<HttpCode>(400);
  }

  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp = this->network.httpPost(token, F("/error"), *json);

#ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    return Option<HttpCode>();
  }

  return Option<HttpCode>(UNWRAP_REF(maybeResp).code);
#else
  return Option<HttpCode>(200);
#endif
}

Result<PlantId, Option<HttpCode>>
Api::registerPlant(const AuthToken &authToken) const noexcept {
  const auto token = authToken.asString();
  const auto mac = this->macAddress();
  this->logger.debugln(F("Register plant. Token: "), token, F(", MAC: "), mac);

  const auto make = [this](JsonDocument &doc) {
    doc["mac"] = this->macAddress();
  };
  const auto maybeJson = this->makeJson<30>(F("Api::registerPlant"), make);
  if (maybeJson.isNone()) {
    return Option<HttpCode>(400);
  }

  const auto &json = UNWRAP_REF(maybeJson);
  const auto maybeResp = this->network.httpPut(token, F("/plant"), *json);

#ifndef IOP_MOCK_MONITOR
  if (maybeResp.isNone()) {
    return Option<HttpCode>();
  }

  const auto &resp = UNWRAP_REF(maybeResp);
  if (resp.code != 200) {
    return Option<HttpCode>(resp.code);
  }

  auto result = PlantId::fromString(resp.payload);
  if (IS_ERR(result)) {
    switch (UNWRAP_ERR(result)) {
    case TOO_BIG:
      const auto sizeStr = std::to_string(resp.payload.length());
      this->logger.errorln(F("Plant Id is too big: size = "), sizeStr);
      return Option<HttpCode>(500);
      break;
    }
    return Option<HttpCode>();
  }
  return UNWRAP_OK(result);
#else
  return PlantId::empty();
#endif
}

extern "C" uint32_t _FS_start;
extern "C" uint32_t _FS_end;

Option<HttpCode> Api::upgrade(const AuthToken &token,
                              const MD5Hash sketchHash) const noexcept {
  // TODO: this is garbate, fix this gambiarra mess
  this->logger.debugln(F("Upgrading sketch"));
  auto maybeHttp = this->network.httpClient(F("/upgrade"), token.asString());

  if (maybeHttp.isNone()) {
    return Option<HttpCode>();
  }
  auto &http = *UNWRAP_REF(maybeHttp);

  // The following code was addapted from ESP8266httpUpdate.h, to allow for
  // authentication and better customization of the upgrade software
  HTTPUpdateResult ret = HTTP_UPDATE_FAILED;

  // use HTTP/1.0 for update since the update handler not support any transfer
  // Encoding
  http.useHTTP10(true);
  http.setTimeout(8000);
  // http.setFollowRedirects(true); // TODO: is this a good decision?
  http.setUserAgent(F("ESP8266-IoP-Update"));
  http.addHeader(F("x-ESP8266-Chip-ID"), String(ESP.getChipId()));
  http.addHeader(F("x-ESP8266-STA-MAC"), WiFi.macAddress());
  http.addHeader(F("x-ESP8266-AP-MAC"), WiFi.softAPmacAddress());
  http.addHeader(F("x-ESP8266-free-space"), String(ESP.getFreeSketchSpace()));
  http.addHeader(F("x-ESP8266-sketch-size"), String(ESP.getSketchSize()));
  http.addHeader(F("x-ESP8266-sketch-md5"), String(ESP.getSketchMD5()));
  http.addHeader(F("x-ESP8266-chip-size"), String(ESP.getFlashChipRealSize()));
  http.addHeader(F("x-ESP8266-sdk-version"), ESP.getSdkVersion());
  http.addHeader(F("x-ESP8266-mode"), F("spiffs"));
  http.addHeader(F("x-ESP8266-version"), sketchHash.asString().get());

  const char *headerkeys[] = {"x-MD5"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);

  // track these headers
  http.collectHeaders(headerkeys, headerkeyssize);

  int code = http.GET();
  int len = http.getSize();

  if (code <= 0) {
    // DEBUG_HTTP_UPDATE("[httpUpdate] HTTP error: %s\n",
    //                  http.errorToString(code).c_str());
    //_setLastError(code);
    http.end();
    return HTTP_UPDATE_FAILED;
  }

  // DEBUG_HTTP_UPDATE("[httpUpdate] Header read fin.\n");
  // DEBUG_HTTP_UPDATE("[httpUpdate] Server header:\n");
  // DEBUG_HTTP_UPDATE("[httpUpdate]  - code: %d\n", code);
  // DEBUG_HTTP_UPDATE("[httpUpdate]  - len: %d\n", len);

  if (http.hasHeader("x-MD5")) {
    // DEBUG_HTTP_UPDATE("[httpUpdate]  - MD5: %s\n",
    //                  http.header("x-MD5").c_str());
  }

  // DEBUG_HTTP_UPDATE("[httpUpdate] ESP8266 info:\n");
  // DEBUG_HTTP_UPDATE("[httpUpdate]  - free Space: %d\n",
  //                  ESP.getFreeSketchSpace());
  // DEBUG_HTTP_UPDATE("[httpUpdate]  - current Sketch Size: %d\n",
  //                  ESP.getSketchSize());

  // if (currentVersion && currentVersion[0] != 0x00) {
  //  DEBUG_HTTP_UPDATE("[httpUpdate]  - current version: %s\n",
  //                    currentVersion.c_str());
  //}

  switch (code) {
  case HTTP_CODE_OK: ///< OK (Start Update)
    if (len > 0) {
      bool startUpdate = true;
      size_t spiffsSize = ((size_t)&_FS_end - (size_t)&_FS_start);
      if (len > (int)spiffsSize) {
        // DEBUG_HTTP_UPDATE("[httpUpdate] spiffsSize to low (%d) needed:
        // %d\n",
        //                  spiffsSize, len);
        startUpdate = false;
      }

      if (!startUpdate) {
        //_setLastError(HTTP_UE_TOO_LESS_SPACE);
        ret = HTTP_UPDATE_FAILED;
      } else {
        WiFiClient *tcp = http.getStreamPtr();

        // if (_closeConnectionsOnUpdate) {
        WiFiUDP::stopAll();
        WiFiClient::stopAllExcept(tcp);
        //}

        delay(100);

        int command;

        command = U_FS;

        if (runUpdate(*tcp, len, http.header("x-MD5"), command)) {
          ret = HTTP_UPDATE_OK;
          // DEBUG_HTTP_UPDATE("[httpUpdate] Update ok\n");
          http.end();

          // if (_rebootOnUpdate) {
          ESP.restart();
          //}
        } else {
          ret = HTTP_UPDATE_FAILED;
          // DEBUG_HTTP_UPDATE("[httpUpdate] Update failed\n");
        }
      }
    } else {
      //_setLastError(HTTP_UE_SERVER_NOT_REPORT_SIZE);
      ret = HTTP_UPDATE_FAILED;
      // DEBUG_HTTP_UPDATE(
      //    "[httpUpdate] Content-Length was 0 or wasn't set by Server?!\n");
    }
    break;
  case HTTP_CODE_NOT_MODIFIED:
    ///< Not Modified (No updates)
    ret = HTTP_UPDATE_NO_UPDATES;
    break;
  case HTTP_CODE_NOT_FOUND:
    //_setLastError(HTTP_UE_SERVER_FILE_NOT_FOUND);
    ret = HTTP_UPDATE_FAILED;
    break;
  case HTTP_CODE_FORBIDDEN:
    //_setLastError(HTTP_UE_SERVER_FORBIDDEN);
    ret = HTTP_UPDATE_FAILED;
    break;
  default:
    //_setLastError(HTTP_UE_SERVER_WRONG_HTTP_CODE);
    ret = HTTP_UPDATE_FAILED;
    // DEBUG_HTTP_UPDATE("[httpUpdate] HTTP Code is (%d)\n", code);
    // http.writeToStream(&Serial1);
    break;
  }

  http.end();
  (void)ret;
  // return ret;

#ifndef IOP_MOCK_MONITOR
  return Option<HttpCode>(200);
#else
  return Result<AuthToken, Option<HttpCode>>(AuthToken::empty());
#endif
}
#endif

#ifdef IOP_API_DISABLED
void Api::setup() const { this->network.setup(); }

bool Api::isConnected() const noexcept { return true; }
String Api::macAddress() const noexcept { return this->network.macAddress(); }
void Api::disconnect() const noexcept {}
LogLevel Api::loggerLevel() const noexcept { return this->logger.level(); }

Option<HttpCode> Api::upgrade(const AuthToken &token,
                              const MD5Hash sketchHash) const noexcept {
  (void)token;
  (void)sketchHash;
  return Option<HttpCode>(200);
}
Option<HttpCode> Api::registerEvent(const AuthToken &authToken,
                                    const Event &event) const noexcept {
  return Option<HttpCode>(200);
}

Result<AuthToken, Option<HttpCode>>
Api::authenticate(const StringView username,
                  const StringView password) const noexcept {
  return Result<AuthToken, Option<HttpCode>>(AuthToken::empty());
}

Result<PlantId, Option<HttpCode>>
Api::registerPlant(const AuthToken &authToken) const noexcept {
  return Result<PlantId, Option<HttpCode>>(PlantId::empty());
}
#endif