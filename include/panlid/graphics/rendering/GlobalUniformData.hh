#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace panlid::graphics::rendering {

// Mirrors sh_GlobalUniforms.h in std140 layout. Keep the two in sync.
// Booleans are 4 bytes, and a mat3 is three vec4 rows (each row is a padded column of the glm
// matrix).
struct GlobalUniformData {
    std::uint32_t BackbufferDraw{};
    std::uint32_t IsDepthRangeZeroToOne{};
    std::uint32_t IsClipSpaceYInverted{};
    std::uint32_t IsUvOriginTopLeft{};

    glm::mat4 ProjMatrix{};
    std::array<glm::vec4, 3> ToMaskingSpace{};
    std::uint32_t IsMasking{};
    float CornerRadius{};
    float CornerExponent{};
    float Padding2{};

    glm::vec4 MaskingRect{};
    float BorderThickness{};
    std::array<float, 3> Padding3{};

    glm::mat4 BorderColour{};
    float MaskingBlendRange{};
    float AlphaExponent{};
    glm::vec2 EdgeOffset{};
    std::uint32_t DiscardInner{};
    float InnerCornerRadius{};
    std::int32_t WrapModeS{};
    std::int32_t WrapModeT{};

    [[nodiscard]] bool operator==(const GlobalUniformData&) const = default;
};

static_assert(offsetof(GlobalUniformData, ProjMatrix) == 16);
static_assert(offsetof(GlobalUniformData, ToMaskingSpace) == 80);
static_assert(offsetof(GlobalUniformData, IsMasking) == 128);
static_assert(offsetof(GlobalUniformData, MaskingRect) == 144);
static_assert(offsetof(GlobalUniformData, BorderThickness) == 160);
static_assert(offsetof(GlobalUniformData, BorderColour) == 176);
static_assert(offsetof(GlobalUniformData, MaskingBlendRange) == 240);
static_assert(offsetof(GlobalUniformData, EdgeOffset) == 248);
static_assert(offsetof(GlobalUniformData, DiscardInner) == 256);
static_assert(sizeof(GlobalUniformData) == 272);

} // namespace panlid::graphics::rendering
