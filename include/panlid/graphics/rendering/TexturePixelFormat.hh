#pragma once

#include <cstdint>

namespace panlid::graphics::rendering {

enum class TexturePixelFormat : std::uint8_t {
    R8G8B8A8Float,
    R16Float,
};

} // namespace panlid::graphics::rendering
