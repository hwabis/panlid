#pragma once

#include <cstdint>

namespace panlid::graphics::textures {

enum class WrapMode : std::uint8_t {
    None,
    ClampToEdge,
    ClampToBorder,
    Repeat,
};

}
