#ifndef IOP_TRACER_HPP
#define IOP_TRACER_HPP

#include "certificate_storage.hpp"

#include "WString.h"
#include <cstdint>

#define IOP_TRACE() IOP_TRACE_INNER(__COUNTER__)

// Some technobabble/shenanigans to expand __COUNTER__ into a string
#define IOP_TRACE_INNER(x) IOP_TRACE_INNER2(x)
#define IOP_TRACE_INNER2(x)                                                    \
  const Tracer iop_tracer_##x(FPSTR(__FILE__), (uint32_t)__LINE__,             \
                              (const char *)__PRETTY_FUNCTION__);

class Tracer {
  // We use raw pointers to avoid circular dependency
  const __FlashStringHelper *file;
  const uint32_t line;
  const char *func;
  const __FlashStringHelper *scope;

public:
  Tracer(const __FlashStringHelper *file, uint32_t line,
         const char *func) noexcept;
  Tracer(const __FlashStringHelper *file, uint32_t line, const char *func,
         const __FlashStringHelper *scope) noexcept;
  ~Tracer() noexcept;
  Tracer(const Tracer &other) noexcept = delete;
  Tracer(Tracer &&other) noexcept = delete;
  auto operator=(const Tracer &other) noexcept -> Tracer & = delete;
  auto operator=(Tracer &&other) noexcept -> Tracer & = delete;
};

#endif