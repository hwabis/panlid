#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class RenderBufferFormat : std::uint8_t {
    D16,   // 16-bit depth
    D32,   // 32-bit depth
    D24S8, // 24-bit depth + 8-bit stencil
    D32S8, // 32-bit depth + 8-bit stencil
};

} // namespace panlid::graphics::rendering
