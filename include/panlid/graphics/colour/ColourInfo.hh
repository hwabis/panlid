#pragma once

#include "panlid/graphics/Colour4.hh"
#include "panlid/graphics/colour/SrgbColour.hh"
#include "panlid/graphics/primitives/Quad.hh"
#include <algorithm>
#include <cassert>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <stdexcept>

namespace panlid::graphics {

struct ColourInfo {
    SrgbColour TopLeft{};
    SrgbColour BottomLeft{};
    SrgbColour TopRight{};
    SrgbColour BottomRight{};
    bool HasSingleColour{};

    ColourInfo() = default;

    ColourInfo(const SrgbColour& colour)
        : TopLeft{colour}, BottomLeft{colour}, TopRight{colour}, BottomRight{colour},
          HasSingleColour{true} {}

    ColourInfo(const Colour4& colour) : ColourInfo{SrgbColour{.Srgb = colour}} {}

    [[nodiscard]] static ColourInfo SingleColour(const SrgbColour& colour) {
        return ColourInfo{colour};
    }

    [[nodiscard]] static ColourInfo GradientHorizontal(const SrgbColour& c1, const SrgbColour& c2) {
        ColourInfo result;
        result.TopLeft = result.BottomLeft = c1;
        result.TopRight = result.BottomRight = c2;
        return result;
    }

    [[nodiscard]] static ColourInfo GradientVertical(const SrgbColour& c1, const SrgbColour& c2) {
        ColourInfo result;
        result.TopLeft = result.TopRight = c1;
        result.BottomLeft = result.BottomRight = c2;
        return result;
    }

    [[nodiscard]] std::optional<SrgbColour> TryExtractSingleColour() const {
        if (!HasSingleColour) {
            return std::nullopt;
        }

        return TopLeft;
    }

    [[nodiscard]] SrgbColour Interpolate(const glm::vec2& interp) const {
        return SrgbColour::FromVector((1.F - interp.y) * ((1.F - interp.x) * TopLeft.ToVector() +
                                                          interp.x * TopRight.ToVector()) +
                                      interp.y * ((1.F - interp.x) * BottomLeft.ToVector() +
                                                  interp.x * BottomRight.ToVector()));
    }

    [[nodiscard]] ColourInfo Interpolate(const primitives::Quad& quad) const {
        if (HasSingleColour) {
            return *this;
        }

        ColourInfo result;
        result.TopLeft = Interpolate(quad.TopLeft);
        result.TopRight = Interpolate(quad.TopRight);
        result.BottomLeft = Interpolate(quad.BottomLeft);
        result.BottomRight = Interpolate(quad.BottomRight);
        return result;
    }

    void ApplyChild(const ColourInfo& childColour) {
        if (!HasSingleColour) {
            ApplyChild(childColour, primitives::Quad::FromRectangle(0.F, 0.F, 1.F, 1.F));
            return;
        }

        if (childColour.HasSingleColour) {
            setSingleColour(getSingleColour() * childColour.getSingleColour());
            return;
        }

        HasSingleColour = false;
        BottomLeft = childColour.BottomLeft * TopLeft;
        TopRight = childColour.TopRight * TopLeft;
        BottomRight = childColour.BottomRight * TopLeft;

        // TopLeft last, the lines above still read it.
        TopLeft = childColour.TopLeft * TopLeft;
    }

    void ApplyChild(const ColourInfo& childColour, const primitives::Quad& interp) {
        // Trace.Assert upstream
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
        if (alpha == 1.F) {
            return *this;
        }

        if (auto single = TryExtractSingleColour()) {
            single->MultiplyAlpha(alpha);
            return ColourInfo{*single};
        }

        ColourInfo result = *this;
        result.TopLeft.MultiplyAlpha(alpha);
        result.BottomLeft.MultiplyAlpha(alpha);
        result.TopRight.MultiplyAlpha(alpha);
        result.BottomRight.MultiplyAlpha(alpha);
        return result;
    }

    [[nodiscard]] bool operator==(const ColourInfo& other) const {
        if (!HasSingleColour) {
            if (other.HasSingleColour) {
                return false;
            }

            return TopLeft == other.TopLeft && TopRight == other.TopRight &&
                   BottomLeft == other.BottomLeft && BottomRight == other.BottomRight;
        }

        return other.HasSingleColour && getSingleColour() == other.getSingleColour();
    }

    [[nodiscard]] SrgbColour GetAverageColour() const {
        if (HasSingleColour) {
            return getSingleColour();
        }

        return SrgbColour::FromVector((TopLeft.ToVector() + TopRight.ToVector() +
                                       BottomLeft.ToVector() + BottomRight.ToVector()) /
                                      4.F);
    }

    [[nodiscard]] float GetMaxAlpha() const {
        return std::max({TopLeft.GetAlpha(), TopRight.GetAlpha(), BottomLeft.GetAlpha(),
                         BottomRight.GetAlpha()});
    }

    [[nodiscard]] float GetMinAlpha() const {
        return std::min({TopLeft.GetAlpha(), TopRight.GetAlpha(), BottomLeft.GetAlpha(),
                         BottomRight.GetAlpha()});
    }

    explicit operator SrgbColour() const {
        if (!HasSingleColour) {
            throw std::runtime_error(
                "Attempted to read single colour from multi-colour ColourInfo.");
        }

        return getSingleColour();
    }

    explicit operator Colour4() const { return static_cast<SrgbColour>(*this).Srgb; }

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

} // namespace panlid::graphics
