#ifndef IOP_DRIVER_STRING_STATIC_HPP
#define IOP_DRIVER_STRING_STATIC_HPP

#ifdef IOP_DESKTOP
#define PSTR(x) reinterpret_cast<const char*>(x)

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

#endif