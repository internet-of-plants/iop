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

#define panic_(msg) panic__(msg, CUTE_FILE, CUTE_LINE, CUTE_FUNC)
#define assert_(cond)                                                          \
  if (!(cond))                                                                 \
    panic_(UnsafeRawString(F("Assertion failed: " #cond)));

// For some reason we can't depend on implicit conversions to
// StaticString/StringView here, probably related to forward class declaration?
void panic__(StringView msg, StaticString file, uint32_t line, StringView func)
    __attribute__((noreturn));
void panic__(StaticString msg, StaticString file, uint32_t line,
             StringView func) __attribute__((noreturn));
void panic__(const __FlashStringHelper *msg, StaticString file, uint32_t line,
             StringView func) __attribute__((noreturn));
void panic__(const String &msg, StaticString file, uint32_t line,
             StringView func) __attribute__((noreturn));
void panic__(UnsafeRawString msg, StaticString file, uint32_t line,
             StringView func) __attribute__((noreturn));
void panic__(const std::string &msg, StaticString file, uint32_t line,
             StringView func) __attribute__((noreturn));

#endif