#ifndef IOP_PANIC_H_
#define IOP_PANIC_H_

#include <cstdint>
#include <static_string.hpp>
#include <string_view.hpp>
#define panic_(msg) panic__(msg, FPSTR(__FILE__), (uint32_t) __LINE__, UnsafeRawString((const char*)__PRETTY_FUNCTION__))
#define assert_(cond) if (cond) { panic_(F("Assertion failed")); }
void panic__(const StringView msg, const StaticString file, const uint32_t line, const StringView func) __attribute__((noreturn));
void panic__(const StaticString msg, const StaticString file, const uint32_t line, const StringView func) __attribute__((noreturn));


#endif