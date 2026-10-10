#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class StencilOperation : std::uint8_t {
    /// Set the stencil buffer to 0.
    Zero,
    /// Bitwise invert the stencil buffer.
    Invert,
    /// Do not change the stencil buffer.
    Keep,
    /// Replace the stencil buffer with the StencilInfo::TestValue.
    Replace,
    /// Increase the stencil buffer by 1 if it's lower than the maximum value.
    Increase,
    /// Decrease the stencil buffer by 1 it's higher than 0.
    Decrease,
    /// Increase the stencil buffer by 1 and wrap to 0 if the result is above the maximum value.
    IncreaseWrap,
    /// Decrease the stencil buffer by 1 and wrap to maximum value if the result is below 0.
    DecreaseWrap,
};

}
