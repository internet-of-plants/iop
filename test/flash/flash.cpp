#include "flash.hpp"

#include <unity.h>

void authToken() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeAuthToken();
  TEST_ASSERT(!flash.readAuthToken().has_value());
  flash.writeAuthToken(AuthToken::fromBytesUnsafe(reinterpret_cast<const uint8_t*>("Bruh"), 4));
  TEST_ASSERT(memcmp(iop::unwrap_ref(flash.readAuthToken(), IOP_CTX()).constPtr(), "Bruh", 4) == 0);
  flash.removeAuthToken();
  TEST_ASSERT(!flash.readAuthToken().has_value());
}

void wifiConfig() {
  const Flash flash(iop::LogLevel::WARN);
  flash.setup();
  flash.removeWifiConfig();
  TEST_ASSERT(!flash.readWifiConfig().has_value());
  const auto bruh = reinterpret_cast<const uint8_t*>("Bruh");
  const WifiCredentials creds(NetworkName::fromBytesUnsafe(bruh, 4), NetworkPassword::fromBytesUnsafe(bruh, 4));
  flash.writeWifiConfig(creds);
  TEST_ASSERT(iop::unwrap_ref(flash.readWifiConfig(), IOP_CTX()).ssid.asString().borrow() == creds.ssid.asString().borrow());
  TEST_ASSERT(iop::unwrap_ref(flash.readWifiConfig(), IOP_CTX()).password.asString().borrow() == creds.password.asString().borrow());
  flash.removeWifiConfig();
  TEST_ASSERT(!flash.readWifiConfig().has_value());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(authToken);
    RUN_TEST(wifiConfig);
    UNITY_END();
    return 0;
}