#pragma once

#include <cstdint>

namespace panlid::logging {

enum class LoggingTarget : std::uint8_t {
    Information,
    Runtime,
    Network,
    Performance,
    Database,
    Input,
};

}
