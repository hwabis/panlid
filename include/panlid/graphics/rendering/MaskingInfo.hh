#pragma once

#include "panlid/graphics/colour/ColourInfo.hh"
#include "panlid/graphics/primitives/Quad.hh"
#include "panlid/graphics/primitives/RectangleF.hh"
#include "panlid/graphics/primitives/RectangleI.hh"
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace panlid::graphics::rendering {

struct MaskingInfo {
    primitives::RectangleI ScreenSpaceAABB{};
    primitives::RectangleF MaskingRect{};

    primitives::Quad ConservativeScreenSpaceQuad{};

    glm::mat3 ToMaskingSpace{};

    float CornerRadius{};
    float CornerExponent{};

    float BorderThickness{};
    ColourInfo BorderColour{};

    float BlendRange{};
    float AlphaExponent{};

    glm::vec2 EdgeOffset{};

    bool Hollow{};
    float HollowCornerRadius{};

    [[nodiscard]] bool operator==(const MaskingInfo&) const = default;
};

} // namespace panlid::graphics::rendering
