#pragma once

#include "panlid/graphics/Colour4.hh"

namespace panlid::graphics::rendering {

class ClearInfo {
  public:
    /// The default clear properties, as defined by OpenGL.
    [[nodiscard]] static constexpr ClearInfo GetDefault() { return ClearInfo{}; }

    /// The colour to write to the frame buffer.
    [[nodiscard]] constexpr Colour4 GetColour() const { return colour_; }
    /// The depth to write to the frame buffer.
    [[nodiscard]] constexpr double GetDepth() const { return depth_; }
    /// The stencil value to write to the frame buffer.
    [[nodiscard]] constexpr int GetStencil() const { return stencil_; }

    explicit constexpr ClearInfo(Colour4 colour = Colour4{}, double depth = 1.0, int stencil = 0)
        : colour_(colour), depth_(depth), stencil_(stencil) {}

    constexpr bool operator==(const ClearInfo& other) const = default;

  private:
    Colour4 colour_;
    double depth_;
    int stencil_;
};

} // namespace panlid::graphics::rendering
