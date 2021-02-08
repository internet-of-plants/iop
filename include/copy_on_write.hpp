#ifndef IOP_COW_HPP
#define IOP_COW_HPP

#include "result.hpp"
#include "tracer.hpp"

/// Copy on write string
///
/// Has limitations on the API, as in, it only provides access to the borrowed
/// type This is because
///
/// We could support other types with templates, but we can't ensure they have
/// a borrowed/owned relationship
class CowString {
  // We use `Result` although the semantics isn't right, to avoid reimplementing
  // sum-types here. And we don't need the complexity of std::variant like types
  Result<StringView, String> storage;

public:
  explicit CowString(String str) noexcept : storage(std::move(str)) {}
  explicit CowString(StringView str) noexcept : storage(std::move(str)) {}

  auto borrow() const noexcept -> StringView;

  ~CowString() noexcept = default;
  CowString(CowString const &other) noexcept;
  CowString(CowString &&other) noexcept = default;
  auto operator=(CowString const &other) noexcept -> CowString &;
  auto operator=(CowString &&other) noexcept -> CowString & = default;
};

#endif