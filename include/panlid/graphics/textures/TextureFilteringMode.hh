#pragma once

#include <cstdint>

namespace panlid::graphics::textures {

enum class TextureFilteringMode : std::uint8_t {
    Linear,
    Nearest,
};

}
