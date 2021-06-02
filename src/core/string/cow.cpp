#include "core/string/cow.hpp"

namespace iop {
auto CowString::borrow() const noexcept -> StringView {
  IOP_TRACE();
  if (IS_OK(storage)) {
    return UNWRAP_OK_REF(storage);
  }
  return UNWRAP_ERR_REF(storage);
}

auto CowString::toString() noexcept -> std::string {
  if (IS_OK(this->storage)) {
      return UNWRAP_OK_REF(this->storage).get();
  }
  return UNWRAP_ERR_REF(this->storage);
}

auto CowString::toMut() noexcept -> std::string & {
  if (IS_OK(this->storage)) {
      std::string msg(UNWRAP_OK_MUT(this->storage).get());
      this->storage = iop::Result<StringView, std::string>(std::move(msg));
  }
  return UNWRAP_ERR_MUT(this->storage);
}

CowString::CowString(CowString const &other) noexcept
    : storage(emptyStringView) {
  IOP_TRACE();
  if (IS_OK(other.storage)) {
    this->storage = UNWRAP_OK_REF(other.storage);
  } else {
    this->storage = UNWRAP_ERR_REF(other.storage);
  }
}

// NOLINTNEXTLINE cert-oop54-cpp
auto CowString::operator=(CowString const &other) noexcept -> CowString & {
  IOP_TRACE();
  if (IS_OK(other.storage)) {
    this->storage = UNWRAP_OK_REF(other.storage);
  } else {
    this->storage = UNWRAP_ERR_REF(other.storage);
  }
  return *this;
}
} // namespace iop