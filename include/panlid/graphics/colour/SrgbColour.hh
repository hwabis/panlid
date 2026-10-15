#pragma once

#include "panlid/graphics/Colour4.hh"
#include <glm/vec4.hpp>

namespace panlid::graphics {

struct SrgbColour {
    Colour4 Srgb{};

    [[nodiscard]] static SrgbColour FromVector(const glm::vec4& v) {
        return {.Srgb = Colour4::FromVector(v)};
    }

    [[nodiscard]] Colour4 GetLinear() const { return Srgb.ToLinear(); }

    [[nodiscard]] float GetAlpha() const { return Srgb.A; }

    [[nodiscard]] SrgbColour operator*(const SrgbColour& other) const {
        if (isWhite(*this)) {
            if (GetAlpha() == 1.F) {
                return other;
            }

            return {.Srgb = {.R = other.Srgb.R,
                             .G = other.Srgb.G,
                             .B = other.Srgb.B,
                             .A = GetAlpha() * other.GetAlpha()}};
        }

        if (isWhite(other)) {
            if (other.GetAlpha() == 1.F) {
                return *this;
            }

            return {.Srgb = {
                        .R = Srgb.R, .G = Srgb.G, .B = Srgb.B, .A = GetAlpha() * other.GetAlpha()}};
        }

        return {.Srgb = (GetLinear() * other.GetLinear()).ToSrgb()};
    }

    [[nodiscard]] SrgbColour operator*(float scale) const {
        if (scale == 1.F) {
            return *this;
        }

        const Colour4 linear = GetLinear();
        return {.Srgb = Colour4{.R = linear.R * scale,
                                .G = linear.G * scale,
                                .B = linear.B * scale,
                                .A = linear.A * scale}
                            .ToSrgb()};
    }

    [[nodiscard]] SrgbColour operator/(float scale) const { return *this * (1.F / scale); }

    [[nodiscard]] SrgbColour operator+(const SrgbColour& other) const {
        return {.Srgb = (GetLinear() + other.GetLinear()).ToSrgb()};
    }

    [[nodiscard]] glm::vec4 ToVector() const { return Srgb.GetVector(); }

    void MultiplyAlpha(float alpha) { Srgb.A *= alpha; }

    [[nodiscard]] bool operator==(const SrgbColour&) const = default;

  private:
    [[nodiscard]] static bool isWhite(const SrgbColour& colour) {
        return colour.Srgb.R == 1.F && colour.Srgb.G == 1.F && colour.Srgb.B == 1.F;
    }
};

} // namespace panlid::graphics
