#ifndef IOP_FLASH_HPP
#define IOP_FLASH_HPP

#include "core/log.hpp"
#include "utils.hpp"
#include <optional>

#include "driver/wifi.hpp"

/// Wraps flash memory to provide a safe and ergonomic API
class Flash {
  iop::Log logger;

public:
  explicit Flash(iop::LogLevel logLevel) noexcept
      : logger(logLevel, F("FLASH")) {
    IOP_TRACE();
  }
  static auto setup() noexcept -> void;

  auto readAuthToken() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>>;
  void removeAuthToken() const noexcept;
  void writeAuthToken(const AuthToken &token) const noexcept;

  auto readWifiConfig() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>>;
  void removeWifiConfig() const noexcept;
  void writeWifiConfig(const WifiCredentials &config) const noexcept;

  ~Flash() { IOP_TRACE(); }
  Flash(Flash const &other) noexcept = default;
  Flash(Flash &&other) noexcept = default;
  auto operator=(Flash const &other) -> Flash & = default;
  auto operator=(Flash &&other) -> Flash & = default;
};

#include "utils.hpp"
#ifndef IOP_FLASH
#define IOP_FLASH_DISABLED
#endif
// If we aren't online we will endup writing/removing dummy values, so let's not
// waste writes
#ifndef IOP_ONLINE
#ifndef IOP_DESKTOP
#define IOP_FLASH_DISABLED
#endif
#endif
#ifndef IOP_MONITOR
#ifndef IOP_DESKTOP
#define IOP_FLASH_DISABLED
#endif
#endif

#endif