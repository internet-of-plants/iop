#ifndef IOP_PANIC_H_
#define IOP_PANIC_H_

#include <Arduino.h>
#include <cstdint>

#undef panic
#define panic(msg) panic__(String(msg), String(__FILE__), (uint32_t) __LINE__, String(__PRETTY_FUNCTION__))
#undef assert
#define assert(cond) if (cond) { panic("Assertion failed"); }
void panic__(const String msg, const String file, const uint32_t line, const String func) __attribute__((noreturn));

#endif