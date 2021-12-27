#ifndef IOP_DRIVER_INTERRUPT_HPP
#define IOP_DRIVER_INTERRUPT_HPP

namespace driver {
struct InterruptLock {
  InterruptLock() noexcept;
  ~InterruptLock() noexcept;
  InterruptLock(InterruptLock const &other) noexcept = delete;
  InterruptLock(InterruptLock &&other) noexcept = delete;
  auto operator=(InterruptLock const &other) noexcept -> InterruptLock & = delete;
  auto operator=(InterruptLock &&other) noexcept -> InterruptLock & = delete;
};
}

#endif