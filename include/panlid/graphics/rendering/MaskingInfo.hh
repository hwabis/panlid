#pragma once

#include "panlid/graphics/colour/ColourInfo.hh"
#include "panlid/graphics/primitives/Quad.hh"
#include "panlid/graphics/primitives/RectangleF.hh"
#include "panlid/graphics/primitives/RectangleI.hh"

#include <glm/glm.hpp>

namespace panlid::graphics::rendering {

struct MaskingInfo {
    primitives::RectangleI ScreenSpaceAABB;
    primitives::RectangleF MaskingRect;

    primitives::Quad ConservativeScreenSpaceQuad{};

    // Transforms screen space coordinates to masking space (likely the parent space of the
    // container doing the masking). A shader uses it to decide which pixels to discard.
    glm::mat3 ToMaskingSpace{0.0F};

    float CornerRadius{};
    float CornerExponent{};

    float BorderThickness{};
    colour::ColourInfo BorderColour;

    float BlendRange{};
    float AlphaExponent{};

    glm::vec2 EdgeOffset{};

    bool Hollow{};
    float HollowCornerRadius{};

    [[nodiscard]] bool operator==(const MaskingInfo&) const = default;
};

} // namespace panlid::graphics::rendering
