#ifndef IOP_DRIVER_INTERRUPT_HPP
#define IOP_DRIVER_INTERRUPT_HPP

namespace iop {
struct InterruptLock {
  InterruptLock() noexcept;
  ~InterruptLock() noexcept;
  InterruptLock(InterruptLock const &other) noexcept = default;
  InterruptLock(InterruptLock &&other) noexcept = default;
  auto operator=(InterruptLock const &other) noexcept
      -> InterruptLock & = default;
  auto operator=(InterruptLock &&other) noexcept -> InterruptLock & = default;
};
}

#endif