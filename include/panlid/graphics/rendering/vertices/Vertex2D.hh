#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>

namespace panlid::graphics::rendering::vertices {

struct Vertex2D {
    glm::vec2 Position{};
    Colour4 Colour;

    [[nodiscard]] static constexpr std::array<VertexMember, 2> GetLayout() {
        return {{
            {.Count = 2, .Offset = offsetof(Vertex2D, Position)},
            {.Count = 4, .Offset = offsetof(Vertex2D, Colour)},
        }};
    }

    [[nodiscard]] bool operator==(const Vertex2D&) const = default;
};

static_assert(sizeof(Colour4) == 4 * sizeof(float), "vertex layouts assume Colour4 is four floats");
static_assert(sizeof(Vertex2D) == 6 * sizeof(float));
static_assert(Vertex<Vertex2D>);

} // namespace panlid::graphics::rendering::vertices
