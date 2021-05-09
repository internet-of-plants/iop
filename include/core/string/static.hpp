#ifndef IOP_CORE_STRING_STATIC_HPP
#define IOP_CORE_STRING_STATIC_HPP

#include <stddef.h>
#include <string.h>
#include <string>

#ifdef IOP_DESKTOP
// Is this the best adaptation?
#define PSTR(x) x

class __FlashStringHelper;
#define PROGMEM 
#define FPSTR(pstr_pointer) (reinterpret_cast<const __FlashStringHelper *>(pstr_pointer))
#define F(string_literal) (FPSTR(PSTR(string_literal)))
#define PGM_P const char *
#define strstr_P(a, b) strstr(a, b)
#define strlen_P(a) strlen(a)
#define memmove_P(dest, orig, len) memmove((void *) dest, (const void *) orig, len)
#define strcmp_P(a, b) strcmp(a, b)
#else
#include "WString.h"
#endif

#define PROGMEM_STRING(name, msg)                                              \
  static const char *const PROGMEM name##_progmem_char = msg;                  \
  static const iop::StaticString name(FPSTR(name##_progmem_char));

namespace iop {
class StringView;

/// Helper string that holds a pointer to a string stored in PROGMEM
/// It's here to provide a typesafe way to handle PROGMEM data and to avoid
/// defaulting to String(__FlashStringHelper*) constructor and allocating
/// implicitly.
///
/// It is not compatible with StringView because it requires aligned reads, and
/// things like `strlen` violate it. So we separate.
///
/// Either call a `_P` functions using `.getCharPtr()` to get the regular
/// pointer. Or construct a String with it using `.get()` so it knows to read
/// from PROGMEM
class StaticString {
private:
  const __FlashStringHelper *str;

public:
  // NOLINTNEXTLINE hicpp-explicit-conversions
  StaticString(const __FlashStringHelper *str) noexcept;

  auto get() const noexcept -> const __FlashStringHelper *;
  auto contains(StringView needle) const noexcept -> bool;
  auto contains(StaticString needle) const noexcept -> bool;
  auto length() const noexcept -> size_t;
  auto isEmpty() const noexcept -> bool;
  auto toStdString() const noexcept -> std::string;

  // Be careful when calling this function, if you pass PGM_P to a function that
  // expects a regular char* a hardware exception _may_ happen, PROGMEM data
  // needs to be read with 32 bits alignment, this has caused trouble in the
  // past and may do again. It's why StringView doesn't have a constructor for
  // StaticString
  auto asCharPtr() const noexcept -> PGM_P;

  ~StaticString() noexcept = default;
  StaticString(StaticString const &other) noexcept = default;
  StaticString(StaticString &&other) noexcept = default;
  auto operator=(StaticString const &other) noexcept
      -> StaticString & = default;
  auto operator=(StaticString &&other) noexcept -> StaticString & = default;
};

PROGMEM_STRING(emptyStaticString, "");

} // namespace iop

#endif