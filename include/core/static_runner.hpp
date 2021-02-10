#ifndef IOP_CORE_STATIC_RUNNER_HPP
#define IOP_CORE_STATIC_RUNNER_HPP

#include <functional>

namespace iop {
/// Helper class that executes functions in its constructor, allowing executing
/// arbitrary functions when variables are being defined (before `setup()` for
/// static variables).
///
/// That means other static variable may be defined before, don't assume the
/// ordering of static definitions at runtime. Use this just to run things
/// before `setup()`
struct StaticRunner {
  explicit StaticRunner(std::function<void()> func) noexcept { func(); }
  ~StaticRunner() noexcept = default;
  StaticRunner(StaticRunner const &other) noexcept = default;
  StaticRunner(StaticRunner &&other) noexcept = default;
  auto operator=(StaticRunner const &other) noexcept
      -> StaticRunner & = default;
  auto operator=(StaticRunner &&other) noexcept -> StaticRunner & = default;
};
} // namespace iop
#endif