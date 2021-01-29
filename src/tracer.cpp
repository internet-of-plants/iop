#include "tracer.hpp"

#include "configuration.h"
#include "static_string.hpp"
#include "unsafe_raw_string.hpp"

Tracer::Tracer(const __FlashStringHelper *file, uint32_t line,
               const char *func) noexcept
    : file(file), line(line), func(func), scope(nullptr) {
  if (logLevel > LogLevel::TRACE)
    return;

  Serial.print(F("[TRACE] TRACER: Entering scope from line "));
  Serial.print(line);
  Serial.print(F(" inside function "));
  Serial.print(func);
  Serial.print(F(" at file "));
  Serial.println(file);
  Serial.print(F("[TRACE] TRACER: Free memory: HEAP = "));
  Serial.print(ESP.getFreeHeap());
  Serial.print(F(", STACK = "));
  Serial.println(ESP.getFreeContStack());
  Serial.flush();
}
Tracer::Tracer(const __FlashStringHelper *file, uint32_t line, const char *func,
               const __FlashStringHelper *scope) noexcept
    : file(file), line(line), func(func), scope(scope) {
  if (logLevel > LogLevel::TRACE)
    return;

  Serial.print(F("[TRACE] TRACER: Entering scope "));
  Serial.print(scope);
  Serial.print(F(" from line "));
  Serial.print(line);
  Serial.print(F(" inside function "));
  Serial.print(func);
  Serial.print(F(" at file "));
  Serial.println(file);
  Serial.flush();
}
Tracer::~Tracer() noexcept {
  if (logLevel > LogLevel::TRACE)
    return;

  if (scope == nullptr) {
    Serial.print(F("[TRACE] TRACER: Leaving scope from line "));
    Serial.print(line);
    Serial.print(F(" inside function "));
    Serial.print(func);
    Serial.print(F(" at file "));
    Serial.println(file);
    Serial.flush();
  } else {
    Serial.print(F("[TRACE] TRACER: Leaving scope "));
    Serial.print(scope);
    Serial.print(F(" from line "));
    Serial.print(line);
    Serial.print(F(" inside function "));
    Serial.print(func);
    Serial.print(F(" at file "));
    Serial.println(file);
    Serial.flush();
  }
}