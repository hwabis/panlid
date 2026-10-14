#pragma once

#include <cstdint>

namespace panlid::graphics {

enum class BlendingEquation : std::uint8_t {
    Inherit,
    Add,
    Min,
    Max,
    Subtract,
    ReverseSubtract,
};

}
