#include "driver/log.hpp"
#include "driver/thread.hpp"

namespace driver {
void logSetup(const iop::LogLevel &level) noexcept {}
void logPrint(const iop::StaticString msg) noexcept {}
void logPrint(const iop::StringView msg) noexcept {}
void logFlush() noexcept {}
}