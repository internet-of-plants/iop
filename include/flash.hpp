#ifndef IOP_FLASH_HPP
#define IOP_FLASH_HPP

#include "ESP8266WiFi.h"
#include "log.hpp"
#include "models.hpp"
#include "option.hpp"

/// Wraps flash memory to provide a API that satisfies our storage needs
class Flash {
  Log logger;

public:
  ~Flash() = default;
  explicit Flash(LogLevel logLevel) noexcept : logger(logLevel, F("FLASH")) {}
  Flash(Flash const &other) noexcept = delete;
  Flash(Flash &&other) noexcept = delete;
  auto operator=(Flash const &other) -> Flash & = delete;
  auto operator=(Flash &&other) -> Flash & = delete;
  static auto setup() noexcept -> void;

  auto readAuthToken() const noexcept -> Option<AuthToken>;
  void removeAuthToken() const noexcept;
  void writeAuthToken(const AuthToken &token) const noexcept;

  auto readWifiConfig() const noexcept -> Option<WifiCredentials>;
  void removeWifiConfig() const noexcept;
  void writeWifiConfig(const WifiCredentials &config) const noexcept;
};

#include "utils.hpp"
#ifndef IOP_FLASH
#define IOP_FLASH_DISABLED
#endif
// If we aren't online we will endup writing/removing dummy values, so let's not
// waste writes
#ifndef IOP_ONLINE
#define IOP_FLASH_DISABLED
#endif
#ifndef IOP_MONITOR
#define IOP_FLASH_DISABLED
#endif

#endif