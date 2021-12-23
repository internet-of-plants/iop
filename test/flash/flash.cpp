#include "flash.hpp"

#include <unity.h>

void authToken() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeToken();
  TEST_ASSERT(!flash.token());
  flash.setToken(AuthToken::fromBytesUnsafe(reinterpret_cast<const uint8_t*>("Bruh"), 4));
  TEST_ASSERT(memcmp(flash.token().value().constPtr(), "Bruh", 4) == 0);
  flash.removeToken();
  TEST_ASSERT(!flash.token());
}

void wifiConfig() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeWifi();
  TEST_ASSERT(!flash.wifi());
  const auto bruh = reinterpret_cast<const uint8_t*>("Bruh");
  const WifiCredentials creds(NetworkName::fromBytesUnsafe(bruh, 4), NetworkPassword::fromBytesUnsafe(bruh, 4));
  flash.setWifi(creds);
  TEST_ASSERT(flash.wifi().value().ssid.asString().borrow() == creds.ssid.asString().borrow());
  TEST_ASSERT(flash.wifi().value().password.asString().borrow() == creds.password.asString().borrow());
  flash.removeWifi();
  TEST_ASSERT(!flash.wifi());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(authToken);
    RUN_TEST(wifiConfig);
    UNITY_END();
    return 0;
}