#ifndef IOP_CORE_TRACER_HPP
#define IOP_CORE_TRACER_HPP

#include "core/string/view.hpp"

/// Returns CodePoint object pointing to the caller
/// this is useful to track callers of functions that can panic
#define IOP_CTX() IOP_CODE_POINT()
#define IOP_CODE_POINT() iop::CodePoint(IOP_FILE, IOP_LINE, IOP_FUNC)

/// Logs scope changes to serial if logLevel is set to TRACE
#define IOP_TRACE() IOP_TRACE_INNER(__COUNTER__)
// Technobabble to stringify __COUNTER__
#define IOP_TRACE_INNER(x) IOP_TRACE_INNER2(x)
#define IOP_TRACE_INNER2(x) const iop::Tracer iop_tracer_##x(IOP_CODE_POINT());

namespace iop {
class CodePoint {
  StaticString file_;
  uint32_t line_;
  StringView func_;

public:
  // Use the IOP_CODE_POINT() macro to construct CodePoint
  CodePoint(StaticString file, uint32_t line, StringView func) noexcept
      : file_(std::move(file)), line_(line), func_(std::move(func)) {}
  auto file() const noexcept -> StaticString { return this->file_; }
  auto line() const noexcept -> uint32_t { return this->line_; }
  auto func() const noexcept -> StringView { return this->func_; }

  ~CodePoint() noexcept = default;
  CodePoint(const CodePoint &other) noexcept = default;
  CodePoint(CodePoint &&other) noexcept = default;
  auto operator=(const CodePoint &other) noexcept -> CodePoint & = default;
  auto operator=(CodePoint &&other) noexcept -> CodePoint & = default;
};

/// Tracer objects, that signifies scoping changes. Helps with post-mortemns
/// analysis
///
/// Doesn't use the official logging system because it's supposed to be used
/// only when physically debugging very specific bugs, and is unfit for network
/// logging.
class Tracer {
  CodePoint point;

public:
  explicit Tracer(CodePoint point) noexcept;
  ~Tracer() noexcept;
  Tracer(const Tracer &other) noexcept = delete;
  Tracer(Tracer &&other) noexcept = delete;
  auto operator=(const Tracer &other) noexcept -> Tracer & = delete;
  auto operator=(Tracer &&other) noexcept -> Tracer & = delete;
};

} // namespace iop

#endif