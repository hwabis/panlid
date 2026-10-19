#pragma once

#include <cstdint>

namespace panlid::graphics {

// Colour components that are written to the framebuffer during blending.
enum class BlendingMask : std::uint8_t {
    None = 0,
    Red = 1,
    Green = 1 << 1,
    Blue = 1 << 2,
    Alpha = 1 << 3,
    All = Red | Green | Blue | Alpha,
};

[[nodiscard]] constexpr BlendingMask operator|(BlendingMask lhs, BlendingMask rhs) {
    return static_cast<BlendingMask>(static_cast<std::uint8_t>(lhs) |
                                     static_cast<std::uint8_t>(rhs));
}

[[nodiscard]] constexpr BlendingMask operator&(BlendingMask lhs, BlendingMask rhs) {
    return static_cast<BlendingMask>(static_cast<std::uint8_t>(lhs) &
                                     static_cast<std::uint8_t>(rhs));
}

} // namespace panlid::graphics
