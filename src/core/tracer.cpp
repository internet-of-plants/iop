#include "core/tracer.hpp"
#include "core/log.hpp"

#include <string>
#include "driver/device.hpp"

namespace iop {
Tracer::Tracer(CodePoint point) noexcept : point(std::move(point)) {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Entering new scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n[TRACE] TRACER: Free memory: HEAP = "), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getFreeHeap()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", STACK = "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getFreeContStack()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", BIGGEST BLOCKS SIZE = "), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getMaxFreeBlockSize()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", HEAP FRAGMENTATION = "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(std::to_string(ESP.getHeapFragmentation()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}
Tracer::~Tracer() noexcept {
  if (!Log::isTracing())
    return;

  Log::flush();
  Log::print(F("[TRACE] TRACER: Leaving scope, at line "), LogLevel::TRACE,
             LogType::START);
  Log::print(std::to_string(this->point.line()), LogLevel::TRACE,
             LogType::CONTINUITY);
  Log::print(F(", in function "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.func(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F(", at file "), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(this->point.file(), LogLevel::TRACE, LogType::CONTINUITY);
  Log::print(F("\n"), LogLevel::TRACE, LogType::END);
  Log::flush();
}

} // namespace iop