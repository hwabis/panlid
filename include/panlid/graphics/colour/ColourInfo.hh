#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/colour/SrgbColour.hh"
#include "panlid/graphics/primitives/Quad.hh"
#include <algorithm>
#include <cassert>
#include <glm/glm.hpp>
#include <optional>
#include <stdexcept>

namespace panlid::graphics::colour {

/// ColourInfo contains information about the colours of all 4 vertices of a quad.
/// These colours are always stored in linear space.
struct ColourInfo {
    SrgbColour TopLeft;
    SrgbColour BottomLeft;
    SrgbColour TopRight;
    SrgbColour BottomRight;
    bool HasSingleColour{};

    ColourInfo() = default;
    ColourInfo(const SrgbColour& colour) { setSingleColour(colour); }
    ColourInfo(const Colour4& colour) : ColourInfo(SrgbColour{colour}) {}

    [[nodiscard]] static ColourInfo SingleColour(const SrgbColour& colour) {
        return ColourInfo{colour};
    }

    [[nodiscard]] static ColourInfo GradientHorizontal(const SrgbColour& c1, const SrgbColour& c2) {
        ColourInfo result;
        result.TopLeft = result.BottomLeft = c1;
        result.TopRight = result.BottomRight = c2;
        result.HasSingleColour = false;
        return result;
    }

    [[nodiscard]] static ColourInfo GradientVertical(const SrgbColour& c1, const SrgbColour& c2) {
        ColourInfo result;
        result.TopLeft = result.TopRight = c1;
        result.BottomLeft = result.BottomRight = c2;
        result.HasSingleColour = false;
        return result;
    }

    // Empty if this is a gradient.
    [[nodiscard]] std::optional<SrgbColour> TryExtractSingleColour() const {
        if (HasSingleColour) {
            return TopLeft;
        }
        return std::nullopt;
    }

    [[nodiscard]] SrgbColour Interpolate(glm::vec2 interp) const {
        return SrgbColour::FromVector((1 - interp.y) * ((1 - interp.x) * TopLeft.ToVector() +
                                                        interp.x * TopRight.ToVector()) +
                                      interp.y * ((1 - interp.x) * BottomLeft.ToVector() +
                                                  interp.x * BottomRight.ToVector()));
    }

    // Interpolates across a sub-quad of the unit quad: the colour "cropped" to that quad.
    [[nodiscard]] ColourInfo Interpolate(const primitives::Quad& quad) const {
        if (HasSingleColour) {
            return *this;
        }

        ColourInfo result;
        result.TopLeft = Interpolate(quad.TopLeft);
        result.TopRight = Interpolate(quad.TopRight);
        result.BottomLeft = Interpolate(quad.BottomLeft);
        result.BottomRight = Interpolate(quad.BottomRight);
        result.HasSingleColour = false;
        return result;
    }

    void ApplyChild(const ColourInfo& childColour) {
        if (!HasSingleColour) {
            ApplyChild(childColour, primitives::Quad::FromRectangle(0, 0, 1, 1));
            return;
        }

        if (childColour.HasSingleColour) {
            setSingleColour(getSingleColour() * childColour.getSingleColour());
        } else {
            HasSingleColour = false;
            BottomLeft = childColour.BottomLeft * TopLeft;
            TopRight = childColour.TopRight * TopLeft;
            BottomRight = childColour.BottomRight * TopLeft;

            // TopLeft last: the lines above still need its original value.
            TopLeft = childColour.TopLeft * TopLeft;
        }
    }

    void ApplyChild(const ColourInfo& childColour, const primitives::Quad& interp) {
        assert(!HasSingleColour);

        const SrgbColour newTopLeft = Interpolate(interp.TopLeft) * childColour.TopLeft;
        const SrgbColour newTopRight = Interpolate(interp.TopRight) * childColour.TopRight;
        const SrgbColour newBottomLeft = Interpolate(interp.BottomLeft) * childColour.BottomLeft;
        const SrgbColour newBottomRight = Interpolate(interp.BottomRight) * childColour.BottomRight;

        TopLeft = newTopLeft;
        TopRight = newTopRight;
        BottomLeft = newBottomLeft;
        BottomRight = newBottomRight;
    }

    [[nodiscard]] static ColourInfo Multiply(const ColourInfo& first, const ColourInfo& second) {
        ColourInfo result;
        result.TopLeft = first.TopLeft * second.TopLeft;
        result.BottomLeft = first.BottomLeft * second.BottomLeft;
        result.TopRight = first.TopRight * second.TopRight;
        result.BottomRight = first.BottomRight * second.BottomRight;
        return result;
    }

    [[nodiscard]] ColourInfo MultiplyAlpha(float alpha) const {
        if (alpha == 1.0F) {
            return *this;
        }

        if (auto single = TryExtractSingleColour()) {
            single->MultiplyAlpha(alpha);
            return *single;
        }

        ColourInfo result = *this;
        result.TopLeft.MultiplyAlpha(alpha);
        result.BottomLeft.MultiplyAlpha(alpha);
        result.TopRight.MultiplyAlpha(alpha);
        result.BottomRight.MultiplyAlpha(alpha);
        return result;
    }

    [[nodiscard]] bool operator==(const ColourInfo&) const = default;

    [[nodiscard]] SrgbColour GetAverageColour() const {
        if (HasSingleColour) {
            return getSingleColour();
        }

        return SrgbColour::FromVector((TopLeft.ToVector() + TopRight.ToVector() +
                                       BottomLeft.ToVector() + BottomRight.ToVector()) /
                                      4.0F);
    }

    [[nodiscard]] float GetMaxAlpha() const {
        return std::max({TopLeft.GetAlpha(), TopRight.GetAlpha(), BottomLeft.GetAlpha(),
                         BottomRight.GetAlpha()});
    }

    [[nodiscard]] float GetMinAlpha() const {
        return std::min({TopLeft.GetAlpha(), TopRight.GetAlpha(), BottomLeft.GetAlpha(),
                         BottomRight.GetAlpha()});
    }

    [[nodiscard]] explicit operator SrgbColour() const {
        if (!HasSingleColour) {
            throw std::logic_error("Attempted to read single colour from multi-colour ColourInfo.");
        }
        return TopLeft;
    }

    [[nodiscard]] explicit operator Colour4() const { return static_cast<SrgbColour>(*this).Srgb; }

  private:
    [[nodiscard]] SrgbColour getSingleColour() const {
        assert(HasSingleColour);
        return TopLeft;
    }

    void setSingleColour(const SrgbColour& value) {
        TopLeft = BottomLeft = TopRight = BottomRight = value;
        HasSingleColour = true;
    }
};

} // namespace panlid::graphics::colour
