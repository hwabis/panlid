#pragma once

#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>

namespace panlid::graphics::rendering::vertices {

struct UncolouredVertex2D {
    glm::vec2 Position{};

    [[nodiscard]] static constexpr std::array<VertexMember, 1> GetLayout() {
        return {{
            {.Count = 2, .Offset = offsetof(UncolouredVertex2D, Position)},
        }};
    }

    [[nodiscard]] bool operator==(const UncolouredVertex2D&) const = default;
};

static_assert(sizeof(UncolouredVertex2D) == 2 * sizeof(float));
static_assert(Vertex<UncolouredVertex2D>);

} // namespace panlid::graphics::rendering::vertices
