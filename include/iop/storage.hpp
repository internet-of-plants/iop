#ifndef IOP_STORAGE_HPP
#define IOP_STORAGE_HPP

#include "iop-hal/log.hpp"
#include "iop/utils.hpp"
#include <optional>

namespace iop {
/// Wraps storage memory to provide a safe and ergonomic API
class Storage {
  iop::Log logger;

public:
  explicit Storage(iop::LogLevel logLevel) noexcept: logger(logLevel, IOP_STR("STORAGE")) {}

  /// Initializes storage memory storage
  static auto setup() noexcept -> void;

  auto token() noexcept -> std::optional<std::reference_wrapper<const AuthToken>>;
  void removeToken() noexcept;
  void setToken(const AuthToken &token) noexcept;

  auto wifi() noexcept -> std::optional<std::reference_wrapper<const WifiCredentials>>;
  void removeWifi() noexcept;
  void setWifi(const WifiCredentials &config) noexcept;
};
}
#endif