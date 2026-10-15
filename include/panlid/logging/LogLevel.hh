#pragma once

#include <cstdint>

namespace panlid::logging {

enum class LogLevel : std::uint8_t {
    Debug,
    Verbose,
    Important,
    Error,
};

}
