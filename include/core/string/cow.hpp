#ifndef IOP_CORE_STRING_COW_HPP
#define IOP_CORE_STRING_COW_HPP

#include "core/result.hpp"

namespace iop {
/// Copy on write string
///
/// Has limitations on the API, as in, it only provides access to the borrowed
/// type This is because
///
/// We could support other types with templates, but we can't ensure they have
/// a borrowed/owned relationship
class CowString {
  // We use `iop::Result` although the semantics isn't right, to avoid
  // reimplementing sum-types here. And we don't need the complexity of
  // std::variant like types.
  iop::Result<StringView, std::string> storage;

public:
  explicit CowString(std::string str) noexcept : storage(std::move(str)) {}
  explicit CowString(StringView str) noexcept : storage(std::move(str)) {}

  auto borrow() const noexcept -> StringView;
  auto get() const noexcept -> const char * { return this->borrow().get(); }
  auto toMut() noexcept -> std::string &;

  ~CowString() noexcept = default;
  CowString(CowString const &other) noexcept;
  CowString(CowString &&other) noexcept = default;
  auto operator=(CowString const &other) noexcept -> CowString &;
  auto operator=(CowString &&other) noexcept -> CowString & = default;
};
} // namespace iop

#endif