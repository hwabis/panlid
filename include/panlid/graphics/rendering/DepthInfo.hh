#pragma once

#include "panlid/graphics/rendering/BufferTestFunction.hh"

namespace panlid::graphics::rendering {

struct DepthInfo {
    bool DepthTest = true;
    bool WriteDepth = true;
    BufferTestFunction Function = BufferTestFunction::LessThan;

    [[nodiscard]] static constexpr DepthInfo Default() { return {}; }

    [[nodiscard]] bool operator==(const DepthInfo&) const = default;
};

} // namespace panlid::graphics::rendering
