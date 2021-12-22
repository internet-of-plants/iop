#include "flash.hpp"

#include <unity.h>

void authToken() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeAuthToken();
  TEST_ASSERT(!flash.readAuthToken());
  flash.writeAuthToken(AuthToken::fromBytesUnsafe(reinterpret_cast<const uint8_t*>("Bruh"), 4));
  TEST_ASSERT(memcmp(flash.readAuthToken().value().constPtr(), "Bruh", 4) == 0);
  flash.removeAuthToken();
  TEST_ASSERT(!flash.readAuthToken());
}

void wifiConfig() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeWifiConfig();
  TEST_ASSERT(!flash.readWifiConfig());
  const auto bruh = reinterpret_cast<const uint8_t*>("Bruh");
  const WifiCredentials creds(NetworkName::fromBytesUnsafe(bruh, 4), NetworkPassword::fromBytesUnsafe(bruh, 4));
  flash.writeWifiConfig(creds);
  TEST_ASSERT(flash.readWifiConfig().value().ssid.asString().borrow() == creds.ssid.asString().borrow());
  TEST_ASSERT(flash.readWifiConfig().value().password.asString().borrow() == creds.password.asString().borrow());
  flash.removeWifiConfig();
  TEST_ASSERT(!flash.readWifiConfig());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(authToken);
    RUN_TEST(wifiConfig);
    UNITY_END();
    return 0;
}