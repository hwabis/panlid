#pragma once

#include "panlid/graphics/Colour4.hh"

namespace panlid::graphics::rendering {

struct ClearInfo {
    Colour4 Colour;
    double Depth = 1.0;
    int Stencil{};

    [[nodiscard]] static constexpr ClearInfo Default() { return {}; }

    [[nodiscard]] bool operator==(const ClearInfo&) const = default;
};

} // namespace panlid::graphics::rendering
