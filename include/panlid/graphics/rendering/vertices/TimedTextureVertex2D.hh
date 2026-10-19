#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>

namespace panlid::graphics::rendering::vertices {

struct TimedTexturedVertex2D {
    glm::vec2 Position{};
    Colour4 Colour;
    glm::vec2 TexturePosition{};
    float Time{};

    [[nodiscard]] static constexpr std::array<VertexMember, 4> GetLayout() {
        return {{
            {.Count = 2, .Offset = offsetof(TimedTexturedVertex2D, Position)},
            {.Count = 4, .Offset = offsetof(TimedTexturedVertex2D, Colour)},
            {.Count = 2, .Offset = offsetof(TimedTexturedVertex2D, TexturePosition)},
            {.Count = 1, .Offset = offsetof(TimedTexturedVertex2D, Time)},
        }};
    }

    [[nodiscard]] bool operator==(const TimedTexturedVertex2D&) const = default;
};

static_assert(sizeof(TimedTexturedVertex2D) == 9 * sizeof(float));
static_assert(Vertex<TimedTexturedVertex2D>);

} // namespace panlid::graphics::rendering::vertices
