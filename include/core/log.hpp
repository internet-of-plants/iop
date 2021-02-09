#ifndef IOP_CORE_LOG_HPP
#define IOP_CORE_LOG_HPP

#include "core/string/static.hpp"

namespace iop {
enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRIT, NO_LOG };
enum class LogType { START, CONTINUITY, STARTEND, END };
} // namespace iop

#endif