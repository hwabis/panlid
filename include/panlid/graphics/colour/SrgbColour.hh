#pragma once

#include "panlid/graphics/Colour4.hh"
#include <glm/vec4.hpp>

namespace panlid::graphics::colour {

struct SrgbColour {
    Colour4 Srgb;

    SrgbColour() = default;
    constexpr SrgbColour(const Colour4& srgb) : Srgb(srgb) {}

    [[nodiscard]] Colour4 GetLinear() const { return Srgb.ToLinear(); }
    [[nodiscard]] constexpr float GetAlpha() const { return Srgb.GetA(); }

    constexpr bool operator==(const SrgbColour& other) const = default;

    [[nodiscard]] SrgbColour operator*(const SrgbColour& second) const {
        if (isWhite(*this)) {
            if (GetAlpha() == 1.0F) {
                return second;
            }
            return Colour4(second.Srgb.GetR(), second.Srgb.GetG(), second.Srgb.GetB(),
                           GetAlpha() * second.GetAlpha());
        }

        if (isWhite(second)) {
            if (second.GetAlpha() == 1.0F) {
                return *this;
            }
            return Colour4(Srgb.GetR(), Srgb.GetG(), Srgb.GetB(), GetAlpha() * second.GetAlpha());
        }

        return Colour4(GetLinear().GetVector() * second.GetLinear().GetVector()).ToSrgb();
    }

    [[nodiscard]] SrgbColour operator*(float second) const {
        if (second == 1.0F) {
            return *this;
        }
        return Colour4(GetLinear().GetVector() * second).ToSrgb();
    }

    [[nodiscard]] SrgbColour operator/(float second) const { return *this * (1.0F / second); }

    [[nodiscard]] SrgbColour operator+(const SrgbColour& second) const {
        return Colour4(GetLinear().GetVector() + second.GetLinear().GetVector()).ToSrgb();
    }

    [[nodiscard]] constexpr glm::vec4 ToVector() const { return Srgb.GetVector(); }
    [[nodiscard]] static SrgbColour FromVector(const glm::vec4& v) { return Colour4(v); }

    constexpr void MultiplyAlpha(float alpha) {
        Srgb = Colour4(Srgb.GetR(), Srgb.GetG(), Srgb.GetB(), Srgb.GetA() * alpha);
    }

  private:
    [[nodiscard]] static constexpr bool isWhite(const SrgbColour& colour) {
        return colour.Srgb.GetR() == 1.0F && colour.Srgb.GetG() == 1.0F &&
               colour.Srgb.GetB() == 1.0F;
    }
};

} // namespace panlid::graphics::colour
