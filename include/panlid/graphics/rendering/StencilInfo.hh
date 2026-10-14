#pragma once

#include "panlid/graphics/rendering/BufferTestFunction.hh"
#include "panlid/graphics/rendering/StencilOperation.hh"

namespace panlid::graphics::rendering {

struct StencilInfo {
    // If false, no stencil operation occurs (use BufferTestFunction::Always instead).
    bool StencilTest = true;
    BufferTestFunction TestFunction = BufferTestFunction::Always;
    int TestValue = 1;
    int Mask = 0xff;
    StencilOperation StencilTestFailOperation = StencilOperation::Keep;
    StencilOperation DepthTestFailOperation = StencilOperation::Keep;
    StencilOperation TestPassedOperation = StencilOperation::Replace;

    [[nodiscard]] static constexpr StencilInfo Default() { return {.StencilTest = false}; }

    [[nodiscard]] bool operator==(const StencilInfo&) const = default;
};

} // namespace panlid::graphics::rendering
