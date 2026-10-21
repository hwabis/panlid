#pragma once

#include <cstdint>

namespace panlid::graphics {

// One 8-bit-per-channel RGBA pixel, as stored in texture uploads.
// todo: image loading and saving (ImageSharp's Image<Rgba32>) needs a library choice, e.g.
// stb_image or SDL3_image.
struct Rgba32 {
    std::uint8_t R{};
    std::uint8_t G{};
    std::uint8_t B{};
    std::uint8_t A{};

    [[nodiscard]] bool operator==(const Rgba32&) const = default;
};

static_assert(sizeof(Rgba32) == 4);

} // namespace panlid::graphics
