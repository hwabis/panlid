#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class BufferTestFunction : std::uint8_t {
    /// The test always fails.
    Never,
    /// The test passes when the incoming value is less than the value in the buffer.
    LessThan,
    /// The test passes when the incoming value is less than or equal to the value in the buffer.
    LessThanOrEqual,
    /// The test passes when the incoming value equals the value in the buffer.
    Equal,
    /// The test passes when the incoming value is greater than or equal to the value in the buffer.
    GreaterThanOrEqual,
    /// The test passes when the incoming value is greater than the value in the buffer.
    GreaterThan,
    /// The test passes when the incoming value is not equal to the value in the buffer.
    NotEqual,
    /// The test always passes.
    Always,
};

}
