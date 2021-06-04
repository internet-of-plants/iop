#include "core/string/cow.hpp"
#include "core/tracer.hpp"
#include "core/utils.hpp"

namespace iop {
auto CowString::borrow() const noexcept -> std::string_view {
  IOP_TRACE();
  if (iop::is_ok(this->storage))
    return iop::unwrap_ok_ref(this->storage, IOP_CTX());
  return iop::unwrap_err_ref(this->storage, IOP_CTX());
}

auto CowString::toString() const noexcept -> std::string {
  return std::string(this->borrow());
}

auto CowString::toMut() noexcept -> std::string & {
  if (iop::is_ok(this->storage))
      this->storage = std::string(iop::unwrap_ok_ref(this->storage, IOP_CTX()));
  return iop::unwrap_err_mut(this->storage, IOP_CTX());
}

CowString::CowString(CowString const &other) noexcept
    : storage(std::string_view()) {
  IOP_TRACE();
  if (iop::is_ok(other.storage)) {
    this->storage = iop::unwrap_ok_ref(other.storage, IOP_CTX());
  } else {
    this->storage = iop::unwrap_err_ref(other.storage, IOP_CTX());
  }
}

// NOLINTNEXTLINE cert-oop54-cpp
auto CowString::operator=(CowString const &other) noexcept -> CowString & {
  IOP_TRACE();
  if (iop::is_ok(other.storage)) {
    this->storage = iop::unwrap_ok_ref(other.storage, IOP_CTX());
  } else {
    this->storage = iop::unwrap_err_ref(other.storage, IOP_CTX());
  }
  return *this;
}
} // namespace iop