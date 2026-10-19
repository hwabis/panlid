#pragma once

#include <cstddef>
#include <cstdint>

namespace panlid::graphics::rendering::vertices {

// todo: C# uses a GL enum here (VertexAttribPointerType). Every vertex uses Float so far; add the
// other component types when one needs them.
enum class VertexComponentType : std::uint8_t {
    Float,
};

// Describes one field of a vertex. Replaces C#'s VertexMemberAttribute: the offset comes from
// offsetof instead of being computed by reflection at startup.
struct VertexMember {
    // Number of components, e.g. a glm::vec2 is 2 Float components.
    int Count{};
    VertexComponentType Type = VertexComponentType::Float;
    // If true, the member is mapped to [-1, 1] (signed) or [0, 1] (unsigned) when passed to the
    // shader.
    bool Normalized{};
    std::size_t Offset{};
};

} // namespace panlid::graphics::rendering::vertices
