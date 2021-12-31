#ifndef IOP_FLASH_HPP
#define IOP_FLASH_HPP

#include "driver/log.hpp"
#include "utils.hpp"

/// Wraps flash memory to provide a safe and ergonomic API
class Flash {
  iop::Log logger;

public:
  explicit Flash(iop::LogLevel logLevel) noexcept: logger(logLevel, IOP_STATIC_STRING("FLASH")) {}

  /// Initializes flash memory storage
  static auto setup() noexcept -> void;

  auto token() const noexcept -> std::optional<std::reference_wrapper<const AuthToken>>;
  void removeToken() const noexcept;
  void setToken(const AuthToken &token) const noexcept;

  auto wifi() const noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>>;
  void removeWifi() const noexcept;
  void setWifi(const WifiCredentials &config) const noexcept;
};

#ifndef IOP_FLASH
#define IOP_FLASH_DISABLED
#endif

#endif