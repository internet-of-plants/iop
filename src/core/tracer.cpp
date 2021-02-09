#include "core/tracer.hpp"

#include "configuration.h"

namespace iop {
Tracer::Tracer(CodePoint c) noexcept : point(std::move(c)) {
  if (logLevel > LogLevel::TRACE)
    return;

  Serial.flush();
  Serial.print(F("[TRACE] TRACER: Entering scope from line "));
  Serial.print(this->point.line());
  Serial.print(F(" inside function "));
  Serial.print(this->point.func().get());
  Serial.print(F(" at file "));
  Serial.println(this->point.file().get());
  Serial.print(F("[TRACE] TRACER: Free memory: HEAP = "));
  Serial.print(ESP.getFreeHeap());
  Serial.print(F(", STACK = "));
  Serial.print(ESP.getFreeContStack());
  Serial.print(F(", BIGGEST BLOCKS SIZE ="));
  Serial.print(ESP.getMaxFreeBlockSize());
  Serial.print(F(", HEAP FRAGMENTATION ="));
  Serial.println(ESP.getHeapFragmentation());
  Serial.flush();
}
Tracer::~Tracer() noexcept {
  if (logLevel > LogLevel::TRACE)
    return;

  Serial.flush();
  Serial.print(F("[TRACE] TRACER: Leaving scope from line "));
  Serial.print(this->point.line());
  Serial.print(F(" inside function "));
  Serial.print(this->point.func().get());
  Serial.print(F(" at file "));
  Serial.println(this->point.file().get());
  Serial.flush();
}

} // namespace iop