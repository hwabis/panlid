#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class BufferTestFunction : std::uint8_t {
    Never,
    LessThan,
    LessThanOrEqual,
    Equal,
    GreaterThanOrEqual,
    GreaterThan,
    NotEqual,
    Always,
};

}
