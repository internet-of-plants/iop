#ifndef IOP_PANIC_HPP
#define IOP_PANIC_HPP

#include "WString.h"

#include <cstdint>
#include <string>

class StringView;
class StaticString;
class UnsafeRawString;

#define CUTE_FILE StaticString(FPSTR(__FILE__))
#define CUTE_LINE (uint32_t) __LINE__
#define CUTE_FUNC StringView(UnsafeRawString((const char *)__PRETTY_FUNCTION__))

/// Panic. Never returns. Logs panic to network if available (and serial).
/// If any update is available it's installed and reboots.
///
/// Sent: Message + Line + Function + File
#define panic_(msg) panic__(msg, CUTE_FILE, CUTE_LINE, CUTE_FUNC)

/// Calls `panic_` with provided message if condition is false
#define assert_(cond, msg)                                                     \
  if (!(cond))                                                                 \
    panic_(msg);

// TODO: For some reason we can't depend on implicit conversions to
// StaticString/StringView here, probably related to forward class
// declaration?
void panic__(StringView msg, const StaticString &file, uint32_t line,
             const StringView &func) __attribute__((noreturn));
void panic__(StaticString msg, const StaticString &file, uint32_t line,
             const StringView &func) __attribute__((noreturn));
void panic__(const __FlashStringHelper *msg, const StaticString &file,
             uint32_t line, const StringView &func) __attribute__((noreturn));
void panic__(const String &msg, const StaticString &file, uint32_t line,
             const StringView &func) __attribute__((noreturn));
void panic__(UnsafeRawString msg, const StaticString &file, uint32_t line,
             const StringView &func) __attribute__((noreturn));
void panic__(const std::string &msg, const StaticString &file, uint32_t line,
             const StringView &func) __attribute__((noreturn));

#endif