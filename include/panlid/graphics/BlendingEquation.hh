#pragma once

#include <cstdint>

namespace panlid::graphics {

enum class BlendingEquation : std::uint8_t {
    /// Inherits from parent.
    Inherit = 0,
    /// Adds the source and destination colours.
    Add,
    /// Chooses the minimum of each component of the source and destination colours.
    Min,
    /// Chooses the maximum of each component of the source and destination colours.
    Max,
    /// Subtracts the destination colour from the source colour.
    Subtract,
    /// Subtracts the source colour from the destination colour.
    ReverseSubtract,
};

}
