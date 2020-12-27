#ifndef IOP_PANIC_H_
#define IOP_PANIC_H_

#include <WString.h>
#include <cstdint>

class StringView;
class StaticString;
class UnsafeRawString;

#define panic_(msg) panic__(msg, StaticString(FPSTR(__FILE__)), (uint32_t) __LINE__, StringView(UnsafeRawString((const char*)__PRETTY_FUNCTION__)))
#define assert_(cond) if (cond) { panic_(StringView(UnsafeRawString("Assertion failed: " #cond))); }

void panic__(const __FlashStringHelper * msg, const StaticString & file, const uint32_t line, const StringView & func) __attribute__((noreturn));
void panic__(const StringView & msg, const StaticString & file, const uint32_t line, const StringView & func) __attribute__((noreturn));
void panic__(const StaticString & msg, const StaticString & file, const uint32_t line, const StringView & func) __attribute__((noreturn));

#endif