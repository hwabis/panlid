#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/rendering/vertices/Vertex.hh"
#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <array>
#include <cstddef>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace panlid::graphics::rendering::vertices {

struct TexturedVertex2D {
    glm::vec2 Position{};
    Colour4 Colour;
    glm::vec2 TexturePosition{};
    glm::vec4 TextureRect{};
    glm::vec2 BlendRange{};
    // Set from the renderer's backbuffer depth; leave it alone otherwise.
    // todo: add FromRenderer(const IRenderer&) once IRenderer is ported (C# only allowed
    // construction through the renderer).
    float BackbufferDrawDepth{};

    [[nodiscard]] static constexpr std::array<VertexMember, 6> GetLayout() {
        return {{
            {.Count = 2, .Offset = offsetof(TexturedVertex2D, Position)},
            {.Count = 4, .Offset = offsetof(TexturedVertex2D, Colour)},
            {.Count = 2, .Offset = offsetof(TexturedVertex2D, TexturePosition)},
            {.Count = 4, .Offset = offsetof(TexturedVertex2D, TextureRect)},
            {.Count = 2, .Offset = offsetof(TexturedVertex2D, BlendRange)},
            {.Count = 1, .Offset = offsetof(TexturedVertex2D, BackbufferDrawDepth)},
        }};
    }

    [[nodiscard]] bool operator==(const TexturedVertex2D&) const = default;
};

static_assert(sizeof(TexturedVertex2D) == 15 * sizeof(float));
static_assert(Vertex<TexturedVertex2D>);

} // namespace panlid::graphics::rendering::vertices
