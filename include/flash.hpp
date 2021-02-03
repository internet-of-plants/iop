#ifndef IOP_FLASH_HPP
#define IOP_FLASH_HPP

#include "certificate_storage.hpp"

#include "log.hpp"
#include "models.hpp"
#include "option.hpp"
#include "static_string.hpp"
#include "string_view.hpp"
#include "tracer.hpp"
#include "unsafe_raw_string.hpp"

#include "ESP8266WiFi.h"

/// Wraps flash memory to provide a API that satisfies our storage needs
class Flash {
  Log logger;

public:
  ~Flash() { IOP_TRACE(); }
  explicit Flash(LogLevel logLevel) noexcept : logger(logLevel, F("FLASH")) {
    IOP_TRACE();
  }
  Flash(Flash const &other) noexcept : logger(other.logger) { IOP_TRACE(); }
  Flash(Flash &&other) noexcept = delete;
  auto operator=(Flash const &other) -> Flash & {
    if (this == &other)
      return *this;
    IOP_TRACE();
    this->logger = other.logger;
    return *this;
  }
  auto operator=(Flash &&other) -> Flash & = delete;
  static auto setup() noexcept -> void;

  auto readAuthToken() const noexcept -> const Option<AuthToken> &;
  void removeAuthToken() const noexcept;
  void writeAuthToken(const AuthToken &token) const noexcept;

  auto readWifiConfig() const noexcept -> const Option<WifiCredentials> &;
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