#pragma once

#include "panlid/graphics/transforms/EasingFunction.hh"
#include "panlid/utils/Interpolable.hh"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace panlid::graphics {

struct Colour4 {
    float R{};
    float G{};
    float B{};
    float A{};

    [[nodiscard]] static constexpr Colour4 FromBytes(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                                     std::uint8_t a) {
        return {.R = static_cast<float>(r) / kByteMax,
                .G = static_cast<float>(g) / kByteMax,
                .B = static_cast<float>(b) / kByteMax,
                .A = static_cast<float>(a) / kByteMax};
    }

    [[nodiscard]] static constexpr Colour4 FromVector(const glm::vec4& vector) {
        return {.R = vector.x, .G = vector.y, .B = vector.z, .A = vector.w};
    }

    [[nodiscard]] static constexpr Colour4 FromRgba(std::uint32_t rgba) {
        return FromBytes(static_cast<std::uint8_t>((rgba >> 24) & 0xFF),
                         static_cast<std::uint8_t>((rgba >> 16) & 0xFF),
                         static_cast<std::uint8_t>((rgba >> 8) & 0xFF),
                         static_cast<std::uint8_t>(rgba & 0xFF));
    }

    [[nodiscard]] static constexpr Colour4 FromArgb(std::uint32_t argb) {
        return FromBytes(static_cast<std::uint8_t>((argb >> 16) & 0xFF),
                         static_cast<std::uint8_t>((argb >> 8) & 0xFF),
                         static_cast<std::uint8_t>(argb & 0xFF),
                         static_cast<std::uint8_t>((argb >> 24) & 0xFF));
    }

    // Accepts RGB, RGBA, RRGGBB and RRGGBBAA, each with an optional leading '#'.
    // Throws std::invalid_argument if the string isn't one of those.
    [[nodiscard]] static Colour4 FromHex(std::string_view hex);

    [[nodiscard]] static std::optional<Colour4> TryParseHex(std::string_view hex);

    // All components are in the 0-1 range.
    [[nodiscard]] static Colour4 FromHsv(float hue, float saturation, float value,
                                         float alpha = 1.F);

    [[nodiscard]] static Colour4 FromHsl(float hue, float saturation, float lightness,
                                         float alpha = 1.F);

    [[nodiscard]] constexpr glm::vec4 GetVector() const { return {R, G, B, A}; }

    // The result's alpha is clamped to 1. Throws std::invalid_argument if scalar is negative.
    [[nodiscard]] Colour4 MultiplyAlpha(float scalar) const {
        if (std::signbit(scalar)) {
            throw std::invalid_argument("scalar must not be negative");
        }

        return {.R = R, .G = G, .B = B, .A = std::min(1.F, A * scalar)};
    }

    [[nodiscard]] constexpr Colour4 Opacity(float alpha) const {
        return {.R = R, .G = G, .B = B, .A = std::clamp(alpha, 0.F, 1.F)};
    }

    [[nodiscard]] constexpr Colour4 OpacityFromByte(std::uint8_t alpha) const {
        return Opacity(static_cast<float>(alpha) / kByteMax);
    }

    [[nodiscard]] constexpr Colour4 Clamped() const {
        return {.R = std::clamp(R, 0.F, 1.F),
                .G = std::clamp(G, 0.F, 1.F),
                .B = std::clamp(B, 0.F, 1.F),
                .A = std::clamp(A, 0.F, 1.F)};
    }

    [[nodiscard]] constexpr Colour4 Lighten(float amount) const {
        const float scalar = std::max(1.F, 1.F + amount);
        return Colour4{.R = R * scalar, .G = G * scalar, .B = B * scalar, .A = A}.Clamped();
    }

    [[nodiscard]] constexpr Colour4 Darken(float amount) const {
        const float scalar = std::max(1.F, 1.F + amount);
        return Colour4{.R = R / scalar, .G = G / scalar, .B = B / scalar, .A = A}.Clamped();
    }

    [[nodiscard]] constexpr Colour4 operator*(const Colour4& other) const {
        return {.R = R * other.R, .G = G * other.G, .B = B * other.B, .A = A * other.A};
    }

    // Clamped to 1.
    [[nodiscard]] constexpr Colour4 operator+(const Colour4& other) const {
        return {.R = std::min(R + other.R, 1.F),
                .G = std::min(G + other.G, 1.F),
                .B = std::min(B + other.B, 1.F),
                .A = std::min(A + other.A, 1.F)};
    }

    // Clamped to 0.
    [[nodiscard]] constexpr Colour4 operator-(const Colour4& other) const {
        return {.R = std::max(R - other.R, 0.F),
                .G = std::max(G - other.G, 0.F),
                .B = std::max(B - other.B, 0.F),
                .A = std::max(A - other.A, 0.F)};
    }

    // Clamped to 1. Throws std::invalid_argument if scalar is negative.
    [[nodiscard]] Colour4 operator*(float scalar) const {
        if (std::signbit(scalar)) {
            throw std::invalid_argument("scalar must not be negative");
        }

        return {.R = std::min(R * scalar, 1.F),
                .G = std::min(G * scalar, 1.F),
                .B = std::min(B * scalar, 1.F),
                .A = std::min(A * scalar, 1.F)};
    }

    // Clamped to 1. Throws std::invalid_argument if scalar is zero or negative.
    [[nodiscard]] Colour4 operator/(float scalar) const {
        if (std::signbit(scalar) || scalar == 0.F) {
            throw std::invalid_argument("scalar must be positive");
        }

        return *this * (1.F / scalar);
    }

    [[nodiscard]] bool operator==(const Colour4&) const = default;

    [[nodiscard]] Colour4 ToLinear() const;

    [[nodiscard]] Colour4 ToSrgb() const;

    [[nodiscard]] constexpr std::uint32_t ToRgba() const {
        return (toByte(R) << 24) | (toByte(G) << 16) | (toByte(B) << 8) | toByte(A);
    }

    [[nodiscard]] constexpr std::uint32_t ToArgb() const {
        return (toByte(A) << 24) | (toByte(R) << 16) | (toByte(G) << 8) | toByte(B);
    }

    // Alpha is only written if the colour is translucent, unless alwaysOutputAlpha is set.
    [[nodiscard]] std::string ToHex(bool alwaysOutputAlpha = false) const;

    // Returned as (hue, saturation, value, alpha), all in 0-1 (hue is compressed to match).
    [[nodiscard]] glm::vec4 ToHsv() const;

    [[nodiscard]] glm::vec4 ToHsl() const;

    // Treats the stored values as sRGB and blends in linear space, so gradients are gamma-correct.
    template <transforms::EasingFunction E>
    [[nodiscard]] Colour4 ValueAt(double time, const Colour4& startColour, const Colour4& endColour,
                                  double startTime, double endTime, const E& easing) const {
        if (startColour == endColour) {
            return startColour;
        }

        const double current = time - startTime;
        const double duration = endTime - startTime;

        if (duration == 0 || current == 0) {
            return startColour;
        }

        const Colour4 startLinear = startColour.ToLinear();
        const Colour4 endLinear = endColour.ToLinear();

        const float t =
            std::clamp(static_cast<float>(easing.ApplyEasing(current / duration)), 0.F, 1.F);

        return Colour4{.R = startLinear.R + t * (endLinear.R - startLinear.R),
                       .G = startLinear.G + t * (endLinear.G - startLinear.G),
                       .B = startLinear.B + t * (endLinear.B - startLinear.B),
                       .A = startLinear.A + t * (endLinear.A - startLinear.A)}
            .ToSrgb();
    }

    [[nodiscard]] static constexpr Colour4 Transparent() { return FromBytes(255, 255, 255, 0); }
    [[nodiscard]] static constexpr Colour4 AliceBlue() { return opaque(240, 248, 255); }
    [[nodiscard]] static constexpr Colour4 AntiqueWhite() { return opaque(250, 235, 215); }
    [[nodiscard]] static constexpr Colour4 Aqua() { return opaque(0, 255, 255); }
    [[nodiscard]] static constexpr Colour4 Aquamarine() { return opaque(127, 255, 212); }
    [[nodiscard]] static constexpr Colour4 Azure() { return opaque(240, 255, 255); }
    [[nodiscard]] static constexpr Colour4 Beige() { return opaque(245, 245, 220); }
    [[nodiscard]] static constexpr Colour4 Bisque() { return opaque(255, 228, 196); }
    [[nodiscard]] static constexpr Colour4 Black() { return opaque(0, 0, 0); }
    [[nodiscard]] static constexpr Colour4 BlanchedAlmond() { return opaque(255, 235, 205); }
    [[nodiscard]] static constexpr Colour4 Blue() { return opaque(0, 0, 255); }
    [[nodiscard]] static constexpr Colour4 BlueViolet() { return opaque(138, 43, 226); }
    [[nodiscard]] static constexpr Colour4 Brown() { return opaque(165, 42, 42); }
    [[nodiscard]] static constexpr Colour4 BurlyWood() { return opaque(222, 184, 135); }
    [[nodiscard]] static constexpr Colour4 CadetBlue() { return opaque(95, 158, 160); }
    [[nodiscard]] static constexpr Colour4 Chartreuse() { return opaque(127, 255, 0); }
    [[nodiscard]] static constexpr Colour4 Chocolate() { return opaque(210, 105, 30); }
    [[nodiscard]] static constexpr Colour4 Coral() { return opaque(255, 127, 80); }
    [[nodiscard]] static constexpr Colour4 CornflowerBlue() { return opaque(100, 149, 237); }
    [[nodiscard]] static constexpr Colour4 Cornsilk() { return opaque(255, 248, 220); }
    [[nodiscard]] static constexpr Colour4 Crimson() { return opaque(220, 20, 60); }
    [[nodiscard]] static constexpr Colour4 Cyan() { return opaque(0, 255, 255); }
    [[nodiscard]] static constexpr Colour4 DarkBlue() { return opaque(0, 0, 139); }
    [[nodiscard]] static constexpr Colour4 DarkCyan() { return opaque(0, 139, 139); }
    [[nodiscard]] static constexpr Colour4 DarkGoldenrod() { return opaque(184, 134, 11); }
    [[nodiscard]] static constexpr Colour4 DarkGray() { return opaque(169, 169, 169); }
    [[nodiscard]] static constexpr Colour4 DarkGreen() { return opaque(0, 100, 0); }
    [[nodiscard]] static constexpr Colour4 DarkKhaki() { return opaque(189, 183, 107); }
    [[nodiscard]] static constexpr Colour4 DarkMagenta() { return opaque(139, 0, 139); }
    [[nodiscard]] static constexpr Colour4 DarkOliveGreen() { return opaque(85, 107, 47); }
    [[nodiscard]] static constexpr Colour4 DarkOrange() { return opaque(255, 140, 0); }
    [[nodiscard]] static constexpr Colour4 DarkOrchid() { return opaque(153, 50, 204); }
    [[nodiscard]] static constexpr Colour4 DarkRed() { return opaque(139, 0, 0); }
    [[nodiscard]] static constexpr Colour4 DarkSalmon() { return opaque(233, 150, 122); }
    [[nodiscard]] static constexpr Colour4 DarkSeaGreen() { return opaque(143, 188, 139); }
    [[nodiscard]] static constexpr Colour4 DarkSlateBlue() { return opaque(72, 61, 139); }
    [[nodiscard]] static constexpr Colour4 DarkSlateGray() { return opaque(47, 79, 79); }
    [[nodiscard]] static constexpr Colour4 DarkTurquoise() { return opaque(0, 206, 209); }
    [[nodiscard]] static constexpr Colour4 DarkViolet() { return opaque(148, 0, 211); }
    [[nodiscard]] static constexpr Colour4 DeepPink() { return opaque(255, 20, 147); }
    [[nodiscard]] static constexpr Colour4 DeepSkyBlue() { return opaque(0, 191, 255); }
    [[nodiscard]] static constexpr Colour4 DimGray() { return opaque(105, 105, 105); }
    [[nodiscard]] static constexpr Colour4 DodgerBlue() { return opaque(30, 144, 255); }
    [[nodiscard]] static constexpr Colour4 Firebrick() { return opaque(178, 34, 34); }
    [[nodiscard]] static constexpr Colour4 FloralWhite() { return opaque(255, 250, 240); }
    [[nodiscard]] static constexpr Colour4 ForestGreen() { return opaque(34, 139, 34); }
    [[nodiscard]] static constexpr Colour4 Fuchsia() { return opaque(255, 0, 255); }
    [[nodiscard]] static constexpr Colour4 Gainsboro() { return opaque(220, 220, 220); }
    [[nodiscard]] static constexpr Colour4 GhostWhite() { return opaque(248, 248, 255); }
    [[nodiscard]] static constexpr Colour4 Gold() { return opaque(255, 215, 0); }
    [[nodiscard]] static constexpr Colour4 Goldenrod() { return opaque(218, 165, 32); }
    [[nodiscard]] static constexpr Colour4 Gray() { return opaque(128, 128, 128); }
    [[nodiscard]] static constexpr Colour4 Green() { return opaque(0, 128, 0); }
    [[nodiscard]] static constexpr Colour4 GreenYellow() { return opaque(173, 255, 47); }
    [[nodiscard]] static constexpr Colour4 Honeydew() { return opaque(240, 255, 240); }
    [[nodiscard]] static constexpr Colour4 HotPink() { return opaque(255, 105, 180); }
    [[nodiscard]] static constexpr Colour4 IndianRed() { return opaque(205, 92, 92); }
    [[nodiscard]] static constexpr Colour4 Indigo() { return opaque(75, 0, 130); }
    [[nodiscard]] static constexpr Colour4 Ivory() { return opaque(255, 255, 240); }
    [[nodiscard]] static constexpr Colour4 Khaki() { return opaque(240, 230, 140); }
    [[nodiscard]] static constexpr Colour4 Lavender() { return opaque(230, 230, 250); }
    [[nodiscard]] static constexpr Colour4 LavenderBlush() { return opaque(255, 240, 245); }
    [[nodiscard]] static constexpr Colour4 LawnGreen() { return opaque(124, 252, 0); }
    [[nodiscard]] static constexpr Colour4 LemonChiffon() { return opaque(255, 250, 205); }
    [[nodiscard]] static constexpr Colour4 LightBlue() { return opaque(173, 216, 230); }
    [[nodiscard]] static constexpr Colour4 LightCoral() { return opaque(240, 128, 128); }
    [[nodiscard]] static constexpr Colour4 LightCyan() { return opaque(224, 255, 255); }
    [[nodiscard]] static constexpr Colour4 LightGoldenrodYellow() { return opaque(250, 250, 210); }
    [[nodiscard]] static constexpr Colour4 LightGreen() { return opaque(144, 238, 144); }
    [[nodiscard]] static constexpr Colour4 LightGray() { return opaque(211, 211, 211); }
    [[nodiscard]] static constexpr Colour4 LightPink() { return opaque(255, 182, 193); }
    [[nodiscard]] static constexpr Colour4 LightSalmon() { return opaque(255, 160, 122); }
    [[nodiscard]] static constexpr Colour4 LightSeaGreen() { return opaque(32, 178, 170); }
    [[nodiscard]] static constexpr Colour4 LightSkyBlue() { return opaque(135, 206, 250); }
    [[nodiscard]] static constexpr Colour4 LightSlateGray() { return opaque(119, 136, 153); }
    [[nodiscard]] static constexpr Colour4 LightSteelBlue() { return opaque(176, 196, 222); }
    [[nodiscard]] static constexpr Colour4 LightYellow() { return opaque(255, 255, 224); }
    [[nodiscard]] static constexpr Colour4 Lime() { return opaque(0, 255, 0); }
    [[nodiscard]] static constexpr Colour4 LimeGreen() { return opaque(50, 205, 50); }
    [[nodiscard]] static constexpr Colour4 Linen() { return opaque(250, 240, 230); }
    [[nodiscard]] static constexpr Colour4 Magenta() { return opaque(255, 0, 255); }
    [[nodiscard]] static constexpr Colour4 Maroon() { return opaque(128, 0, 0); }
    [[nodiscard]] static constexpr Colour4 MediumAquamarine() { return opaque(102, 205, 170); }
    [[nodiscard]] static constexpr Colour4 MediumBlue() { return opaque(0, 0, 205); }
    [[nodiscard]] static constexpr Colour4 MediumOrchid() { return opaque(186, 85, 211); }
    [[nodiscard]] static constexpr Colour4 MediumPurple() { return opaque(147, 112, 219); }
    [[nodiscard]] static constexpr Colour4 MediumSeaGreen() { return opaque(60, 179, 113); }
    [[nodiscard]] static constexpr Colour4 MediumSlateBlue() { return opaque(123, 104, 238); }
    [[nodiscard]] static constexpr Colour4 MediumSpringGreen() { return opaque(0, 250, 154); }
    [[nodiscard]] static constexpr Colour4 MediumTurquoise() { return opaque(72, 209, 204); }
    [[nodiscard]] static constexpr Colour4 MediumVioletRed() { return opaque(199, 21, 133); }
    [[nodiscard]] static constexpr Colour4 MidnightBlue() { return opaque(25, 25, 112); }
    [[nodiscard]] static constexpr Colour4 MintCream() { return opaque(245, 255, 250); }
    [[nodiscard]] static constexpr Colour4 MistyRose() { return opaque(255, 228, 225); }
    [[nodiscard]] static constexpr Colour4 Moccasin() { return opaque(255, 228, 181); }
    [[nodiscard]] static constexpr Colour4 NavajoWhite() { return opaque(255, 222, 173); }
    [[nodiscard]] static constexpr Colour4 Navy() { return opaque(0, 0, 128); }
    [[nodiscard]] static constexpr Colour4 OldLace() { return opaque(253, 245, 230); }
    [[nodiscard]] static constexpr Colour4 Olive() { return opaque(128, 128, 0); }
    [[nodiscard]] static constexpr Colour4 OliveDrab() { return opaque(107, 142, 35); }
    [[nodiscard]] static constexpr Colour4 Orange() { return opaque(255, 165, 0); }
    [[nodiscard]] static constexpr Colour4 OrangeRed() { return opaque(255, 69, 0); }
    [[nodiscard]] static constexpr Colour4 Orchid() { return opaque(218, 112, 214); }
    [[nodiscard]] static constexpr Colour4 PaleGoldenrod() { return opaque(238, 232, 170); }
    [[nodiscard]] static constexpr Colour4 PaleGreen() { return opaque(152, 251, 152); }
    [[nodiscard]] static constexpr Colour4 PaleTurquoise() { return opaque(175, 238, 238); }
    [[nodiscard]] static constexpr Colour4 PaleVioletRed() { return opaque(219, 112, 147); }
    [[nodiscard]] static constexpr Colour4 PapayaWhip() { return opaque(255, 239, 213); }
    [[nodiscard]] static constexpr Colour4 PeachPuff() { return opaque(255, 218, 185); }
    [[nodiscard]] static constexpr Colour4 Peru() { return opaque(205, 133, 63); }
    [[nodiscard]] static constexpr Colour4 Pink() { return opaque(255, 192, 203); }
    [[nodiscard]] static constexpr Colour4 Plum() { return opaque(221, 160, 221); }
    [[nodiscard]] static constexpr Colour4 PowderBlue() { return opaque(176, 224, 230); }
    [[nodiscard]] static constexpr Colour4 Purple() { return opaque(128, 0, 128); }
    [[nodiscard]] static constexpr Colour4 Red() { return opaque(255, 0, 0); }
    [[nodiscard]] static constexpr Colour4 RosyBrown() { return opaque(188, 143, 143); }
    [[nodiscard]] static constexpr Colour4 RoyalBlue() { return opaque(65, 105, 225); }
    [[nodiscard]] static constexpr Colour4 SaddleBrown() { return opaque(139, 69, 19); }
    [[nodiscard]] static constexpr Colour4 Salmon() { return opaque(250, 128, 114); }
    [[nodiscard]] static constexpr Colour4 SandyBrown() { return opaque(244, 164, 96); }
    [[nodiscard]] static constexpr Colour4 SeaGreen() { return opaque(46, 139, 87); }
    [[nodiscard]] static constexpr Colour4 SeaShell() { return opaque(255, 245, 238); }
    [[nodiscard]] static constexpr Colour4 Sienna() { return opaque(160, 82, 45); }
    [[nodiscard]] static constexpr Colour4 Silver() { return opaque(192, 192, 192); }
    [[nodiscard]] static constexpr Colour4 SkyBlue() { return opaque(135, 206, 235); }
    [[nodiscard]] static constexpr Colour4 SlateBlue() { return opaque(106, 90, 205); }
    [[nodiscard]] static constexpr Colour4 SlateGray() { return opaque(112, 128, 144); }
    [[nodiscard]] static constexpr Colour4 Snow() { return opaque(255, 250, 250); }
    [[nodiscard]] static constexpr Colour4 SpringGreen() { return opaque(0, 255, 127); }
    [[nodiscard]] static constexpr Colour4 SteelBlue() { return opaque(70, 130, 180); }
    [[nodiscard]] static constexpr Colour4 Tan() { return opaque(210, 180, 140); }
    [[nodiscard]] static constexpr Colour4 Teal() { return opaque(0, 128, 128); }
    [[nodiscard]] static constexpr Colour4 Thistle() { return opaque(216, 191, 216); }
    [[nodiscard]] static constexpr Colour4 Tomato() { return opaque(255, 99, 71); }
    [[nodiscard]] static constexpr Colour4 Turquoise() { return opaque(64, 224, 208); }
    [[nodiscard]] static constexpr Colour4 Violet() { return opaque(238, 130, 238); }
    [[nodiscard]] static constexpr Colour4 Wheat() { return opaque(245, 222, 179); }
    [[nodiscard]] static constexpr Colour4 White() { return opaque(255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 WhiteSmoke() { return opaque(245, 245, 245); }
    [[nodiscard]] static constexpr Colour4 Yellow() { return opaque(255, 255, 0); }
    [[nodiscard]] static constexpr Colour4 YellowGreen() { return opaque(154, 205, 50); }

  private:
    static constexpr float kByteMax = 255.F;

    [[nodiscard]] static constexpr std::uint32_t toByte(float value) {
        return static_cast<std::uint32_t>(std::clamp(value, 0.F, 1.F) * kByteMax);
    }

    [[nodiscard]] static constexpr Colour4 opaque(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
        return FromBytes(r, g, b, 255);
    }
};

static_assert(sizeof(Colour4) == 4 * sizeof(float));
static_assert(utils::Interpolable<Colour4>);

} // namespace panlid::graphics
