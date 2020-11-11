#ifndef IOP_PANIC_H_
#define IOP_PANIC_H_

#include <WString.h>
#include <static_string.hpp>
#include <cstdint>

#define panic_(msg) panic__(msg, STATIC_STRING(__FILE__), (uint32_t) __LINE__, StaticString(__PRETTY_FUNCTION__))
#define assert_(cond) if (cond) { panic(STATIC_STRING("Assertion failed")); }
void panic__(const String msg, const StaticString file, const uint32_t line, const StaticString func) __attribute__((noreturn));
void panic__(const StaticString msg, const StaticString file, const uint32_t line, const StaticString func) __attribute__((noreturn));


#endif