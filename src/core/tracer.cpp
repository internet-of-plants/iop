#include "core/tracer.hpp"
#include "core/log.hpp"

#ifndef IOP_DESKTOP
#include "Esp.h"
#endif

namespace iop {
Tracer::Tracer(CodePoint point) noexcept : point(std::move(point)) {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Entering new scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file().get(), LogLevel::TRACE, LogType::CONTINUITY);
  #ifndef IOP_DESKTOP
  Log::print(F("\n[TRACE] TRACER: Free memory: HEAP = "), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getFreeHeap()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", STACK = "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getFreeContStack()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", BIGGEST BLOCKS SIZE = "), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getMaxFreeBlockSize()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", HEAP FRAGMENTATION = "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getHeapFragmentation()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  #endif
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Leaving scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

} // namespace iop