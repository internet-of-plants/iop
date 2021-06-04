#ifndef IOP_CORE_STRING_COW_HPP
#define IOP_CORE_STRING_COW_HPP

#include <variant>
#include <string_view>
#include <string>

namespace iop {
/// Copy on write string
///
/// Has limitations on the API, as in, it only provides access to the borrowed
/// type This is because
///
/// We could support other types with templates, but we can't ensure they have
/// a borrowed/owned relationship
class CowString {
  std::variant<std::string_view, std::string> storage;

public:
  explicit CowString(std::string str) noexcept : storage(std::move(str)) {}
  explicit CowString(std::string_view str) noexcept : storage(std::move(str)) {}

  auto borrow() const noexcept -> std::string_view;
  auto get() const noexcept -> const char * { return this->borrow().begin(); }
  auto toMut() noexcept -> std::string &;
  auto toString() const noexcept -> std::string;

  ~CowString() noexcept = default;
  CowString(CowString const &other) noexcept;
  CowString(CowString &&other) noexcept = default;
  auto operator=(CowString const &other) noexcept -> CowString &;
  auto operator=(CowString &&other) noexcept -> CowString & = default;
};
} // namespace iop

#endif