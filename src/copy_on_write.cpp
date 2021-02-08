#include "copy_on_write.hpp"
#include "unsafe_raw_string.hpp"

auto CowString::borrow() const noexcept -> StringView {
  IOP_TRACE();
  if (IS_OK(storage)) {
    return UNWRAP_OK_REF(storage);
  }
  return UNWRAP_ERR_REF(storage);
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