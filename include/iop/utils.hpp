#ifndef IOP_UTILS_HPP
#define IOP_UTILS_HPP

#include "iop-hal/string.hpp"
#include <functional>

namespace iop {
// If you change the number of interrupt types, please update interruptVariant to the correct size
enum class InterruptEvent { NONE, ON_CONNECTION, MUST_UPGRADE };
constexpr static uint8_t interruptVariants = 3;

namespace panic {
  /// Sets custom panic hook to device, this hook logs the panic to the monitor server and requests for an update from the server constantly, rebooting when it succeeds
  void setup() noexcept;
}
namespace network_logger {
  /// Sets custom logging hook to device, this hook also logs messages, from `iop::LogType::INFO` on, to the monitor server.
  void setup() noexcept;
}

/// Schedules update to run in the next main loop run.
/// Has a pre-allocated slot for every kind of interrupt we support.
/// Discards interrupt if it already is scheduled for the next main loop run.
void scheduleInterrupt(InterruptEvent ev) noexcept;

/// Extracts first interrupt scheduled. Should be called until a `InterruptEvent::NONE` is returned.
auto descheduleInterrupt() noexcept -> InterruptEvent;

/// Represents an authentication token returned by the monitor server.
///
/// Must be sent in every request to the monitor server.
using AuthToken = std::array<char, 64>;

/// Helpful to pass around references to the cached stored WiFi credentials
struct WifiCredentials {
  std::reference_wrapper<const iop::NetworkName> ssid;
  std::reference_wrapper<const iop::NetworkPassword> password;

  WifiCredentials(const iop::NetworkName &ssid, const iop::NetworkPassword &pass) noexcept
      : ssid(std::ref(ssid)), password(std::ref(pass)) {}
};
}
#endif