#pragma once

#include "panlid/logging/LogLevel.hh"
#include "panlid/logging/LoggingTarget.hh"
#include <optional>
#include <string>

namespace panlid::logging {

struct LogEntry {
    LogLevel Level{};
    std::optional<LoggingTarget> Target;
    std::string LoggerName;
    std::string Message;
    std::optional<std::string> Exception;
};

} // namespace panlid::logging
