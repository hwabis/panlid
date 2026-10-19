#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>

namespace panlid::graphics::rendering::vertices {

struct ParticleVertex2D {
    glm::vec2 Position{};
    Colour4 Colour;
    glm::vec2 TexturePosition{};
    float Time{};
    glm::vec2 Direction{};

    [[nodiscard]] static constexpr std::array<VertexMember, 5> GetLayout() {
        return {{
            {.Count = 2, .Offset = offsetof(ParticleVertex2D, Position)},
            {.Count = 4, .Offset = offsetof(ParticleVertex2D, Colour)},
            {.Count = 2, .Offset = offsetof(ParticleVertex2D, TexturePosition)},
            {.Count = 1, .Offset = offsetof(ParticleVertex2D, Time)},
            {.Count = 2, .Offset = offsetof(ParticleVertex2D, Direction)},
        }};
    }

    [[nodiscard]] bool operator==(const ParticleVertex2D&) const = default;
};

static_assert(sizeof(ParticleVertex2D) == 11 * sizeof(float));
static_assert(Vertex<ParticleVertex2D>);

} // namespace panlid::graphics::rendering::vertices
