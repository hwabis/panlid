#pragma once

#include "panlid/graphics/rendering/vertices/VertexMember.hh"
#include <concepts>
#include <ranges>
#include <type_traits>

namespace panlid::graphics::rendering::vertices {

// A type that can be uploaded to the GPU as a vertex: plain bytes, comparable, and able to describe
// its own layout.
template <typename T>
concept Vertex = std::is_trivially_copyable_v<T> && std::equality_comparable<T> && requires {
    { T::GetLayout() } -> std::ranges::range;
    requires std::same_as<std::ranges::range_value_t<decltype(T::GetLayout())>, VertexMember>;
};

} // namespace panlid::graphics::rendering::vertices
