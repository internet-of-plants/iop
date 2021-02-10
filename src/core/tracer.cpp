#include "core/tracer.hpp"
#include "Esp.h"
#include "core/log.hpp"


namespace iop {
Tracer::Tracer(CodePoint c) noexcept : point(std::move(c)) {
  if (!Log::isTracing())
    return;

  // TODO: use logger interface instead of Serial, even if a new tracer hook
  Log::flush();
  Log::print(F("[TRACE] TRACER: Entering scope from line "), LogLevel::TRACE,
             LogType::START);
  Log::print(String(this->point.line()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(" inside function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(" at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n[TRACE] TRACER: Free memory: HEAP = "), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(String(ESP.getFreeHeap()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", STACK = "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(String(ESP.getFreeContStack()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", BIGGEST BLOCKS SIZE ="), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(String(ESP.getMaxFreeBlockSize()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", HEAP FRAGMENTATION ="), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(String(ESP.getHeapFragmentation()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Leaving scope from line "), LogLevel::TRACE,
             LogType::START);
  Log::print(String(this->point.line()).c_str(), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(" inside function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(" at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file().get(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

} // namespace iop