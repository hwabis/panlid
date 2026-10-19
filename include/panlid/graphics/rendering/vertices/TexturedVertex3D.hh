#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace panlid::graphics::rendering::vertices {

struct TexturedVertex3D {
    glm::vec3 Position{};
    Colour4 Colour;
    glm::vec2 TexturePosition{};

    [[nodiscard]] static constexpr std::array<VertexMember, 3> GetLayout() {
        return {{
            {.Count = 3, .Offset = offsetof(TexturedVertex3D, Position)},
            {.Count = 4, .Offset = offsetof(TexturedVertex3D, Colour)},
            {.Count = 2, .Offset = offsetof(TexturedVertex3D, TexturePosition)},
        }};
    }

    [[nodiscard]] bool operator==(const TexturedVertex3D&) const = default;
};

static_assert(sizeof(TexturedVertex3D) == 9 * sizeof(float));
static_assert(Vertex<TexturedVertex3D>);

} // namespace panlid::graphics::rendering::vertices
