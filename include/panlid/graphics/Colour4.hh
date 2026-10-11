#pragma once

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace panlid::graphics {

/// Represents an RGBA colour in the linear colour space, having colour components in the range 0-1.
/// Stored internally as a glm::vec4 for performance.
class Colour4 {
  public:
    /// Vector representation of the colour, where xyzw maps to RGBA, each in the 0-1 range.
    [[nodiscard]] constexpr const glm::vec4& GetVector() const { return vector_; }
    /// The red component, in the 0-1 range.
    [[nodiscard]] constexpr float GetR() const { return vector_.x; }
    /// The green component, in the 0-1 range.
    [[nodiscard]] constexpr float GetG() const { return vector_.y; }
    /// The blue component, in the 0-1 range.
    [[nodiscard]] constexpr float GetB() const { return vector_.z; }
    /// The alpha component, in the 0-1 range.
    [[nodiscard]] constexpr float GetA() const { return vector_.w; }

    /// Creates a Colour4 with all components zero (transparent black).
    constexpr Colour4() = default;

    /// Creates a Colour4 with the specified RGBA components in the 0-1 range.
    constexpr Colour4(float r, float g, float b, float a) : vector_(r, g, b, a) {}

    /// Creates a Colour4 from a vector, where xyzw maps to RGBA.
    explicit constexpr Colour4(const glm::vec4& vector) : vector_(vector) {}

    /// Creates a Colour4 with the specified RGBA components in the 0-255 range.
    [[nodiscard]] static constexpr Colour4 FromBytes(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                                     std::uint8_t a) {
        constexpr float kMax = 255.0F;
        return {static_cast<float>(r) / kMax, static_cast<float>(g) / kMax,
                static_cast<float>(b) / kMax, static_cast<float>(a) / kMax};
    }

    /// Returns a new Colour4 with the same RGB components, but multiplying the current alpha
    /// component by a scalar value. The final alpha is clamped to the 0-1 range.
    [[nodiscard]] Colour4 MultiplyAlpha(float scalar) const {
        if (scalar < 0.0F) {
            throw std::invalid_argument("scalar must not be negative");
        }
        return {GetR(), GetG(), GetB(), std::min(1.0F, GetA() * scalar)};
    }

    /// Same RGB, with a specified alpha in the 0-1 range (clamped).
    [[nodiscard]] Colour4 Opacity(float alpha) const {
        return {GetR(), GetG(), GetB(), std::clamp(alpha, 0.0F, 1.0F)};
    }

    /// Same RGB, with a specified alpha in the 0-255 range.
    [[nodiscard]] Colour4 OpacityFromByte(std::uint8_t alpha) const {
        return {GetR(), GetG(), GetB(), static_cast<float>(alpha) / 255.0F};
    }

    /// A copy with every component clamped to the 0-1 range.
    [[nodiscard]] Colour4 Clamped() const { return Colour4(glm::clamp(vector_, 0.0F, 1.0F)); }

    /// Returns a lightened version of the colour. amount is a percentage of light addition.
    [[nodiscard]] Colour4 Lighten(float amount) const {
        const float scalar = std::max(1.0F, 1.0F + amount);
        return Colour4(GetR() * scalar, GetG() * scalar, GetB() * scalar, GetA()).Clamped();
    }

    /// Returns a darkened version of the colour. amount is a percentage of light reduction.
    [[nodiscard]] Colour4 Darken(float amount) const {
        const float scalar = std::max(1.0F, 1.0F + amount);
        return Colour4(GetR() / scalar, GetG() / scalar, GetB() / scalar, GetA()).Clamped();
    }

    /// Multiplies two colours in the linear colour space.
    [[nodiscard]] Colour4 operator*(const Colour4& second) const {
        return Colour4(vector_ * second.vector_);
    }

    /// Adds two colours in the linear colour space. The final value is clamped to 0-1.
    [[nodiscard]] Colour4 operator+(const Colour4& second) const {
        return Colour4(glm::min(vector_ + second.vector_, 1.0F));
    }

    /// Subtracts two colours in the linear colour space. The final value is clamped to 0-1.
    [[nodiscard]] Colour4 operator-(const Colour4& second) const {
        return Colour4(glm::max(vector_ - second.vector_, 0.0F));
    }

    /// Multiplies a colour by a scalar. The final value is clamped to 0-1.
    /// Throws std::invalid_argument if scalar is negative.
    [[nodiscard]] Colour4 operator*(float scalar) const {
        if (scalar < 0.0F) {
            throw std::invalid_argument("scalar must not be negative");
        }
        return Colour4(glm::min(vector_ * scalar, 1.0F));
    }

    /// Divides a colour by a scalar. The final value is clamped to 0-1.
    /// Throws std::invalid_argument if scalar is zero or negative.
    [[nodiscard]] Colour4 operator/(float scalar) const {
        if (scalar <= 0.0F) {
            throw std::invalid_argument("scalar must be positive");
        }
        return *this * (1.0F / scalar);
    }

    bool operator==(const Colour4& other) const = default;

    /// Applies an sRGB -> linear conversion to each chromatic component. Alpha is unchanged.
    [[nodiscard]] Colour4 ToLinear() const {
        return {toLinear(GetR()), toLinear(GetG()), toLinear(GetB()), GetA()};
    }

    /// Applies a linear -> sRGB conversion to each chromatic component. Alpha is unchanged.
    [[nodiscard]] Colour4 ToSrgb() const {
        return {toSrgb(GetR()), toSrgb(GetG()), toSrgb(GetB()), GetA()};
    }

    /// The colour as a 32-bit unsigned integer in the format RGBA.
    [[nodiscard]] std::uint32_t ToRgba() const {
        return (packByte(GetR()) << 24) | (packByte(GetG()) << 16) | (packByte(GetB()) << 8) |
               packByte(GetA());
    }

    /// Creates a Colour4 from a 32-bit unsigned integer in the format RGBA.
    [[nodiscard]] static Colour4 FromRgba(std::uint32_t rgba) {
        return FromBytes(static_cast<std::uint8_t>((rgba >> 24) & 0xFF),
                         static_cast<std::uint8_t>((rgba >> 16) & 0xFF),
                         static_cast<std::uint8_t>((rgba >> 8) & 0xFF),
                         static_cast<std::uint8_t>(rgba & 0xFF));
    }

    /// The colour as a 32-bit unsigned integer in the format ARGB.
    [[nodiscard]] std::uint32_t ToArgb() const {
        return (packByte(GetA()) << 24) | (packByte(GetR()) << 16) | (packByte(GetG()) << 8) |
               packByte(GetB());
    }

    /// Creates a Colour4 from a 32-bit unsigned integer in the format ARGB.
    [[nodiscard]] static Colour4 FromArgb(std::uint32_t argb) {
        return FromBytes(static_cast<std::uint8_t>((argb >> 16) & 0xFF),
                         static_cast<std::uint8_t>((argb >> 8) & 0xFF),
                         static_cast<std::uint8_t>(argb & 0xFF),
                         static_cast<std::uint8_t>((argb >> 24) & 0xFF));
    }

    /// Converts an RGB or RGBA hex colour code into a Colour4. Supported formats: RGB, #RGB, RGBA,
    /// #RGBA, RRGGBB, #RRGGBB, RRGGBBAA, #RRGGBBAA. Throws std::invalid_argument if hex is not a
    /// supported colour code.
    [[nodiscard]] static Colour4 FromHex(std::string_view hex) {
        const std::optional<Colour4> colour = TryParseHex(hex);
        if (!colour) {
            throw std::invalid_argument(std::string(hex) + " is not a valid colour hex string.");
        }
        return *colour;
    }

    /// Attempts to convert a hex colour code (see FromHex for formats) into a Colour4.
    /// Returns std::nullopt if it could not be parsed.
    [[nodiscard]] static std::optional<Colour4> TryParseHex(std::string_view hex) {
        if (hex.starts_with('#')) {
            hex.remove_prefix(1);
        }

        std::uint8_t r = 255;
        std::uint8_t g = 255;
        std::uint8_t b = 255;
        std::uint8_t a = 255;

        const auto parse = [&hex](std::size_t pos, std::size_t len, std::uint8_t& out) {
            return parseHexByte(hex.substr(pos, len), out);
        };

        switch (hex.size()) {
        case 3:
            if (!(parse(0, 1, r) && parse(1, 1, g) && parse(2, 1, b))) {
                return std::nullopt;
            }
            r = expandNibble(r);
            g = expandNibble(g);
            b = expandNibble(b);
            break;
        case 4:
            if (!(parse(0, 1, r) && parse(1, 1, g) && parse(2, 1, b) && parse(3, 1, a))) {
                return std::nullopt;
            }
            r = expandNibble(r);
            g = expandNibble(g);
            b = expandNibble(b);
            a = expandNibble(a);
            break;
        case 6:
            if (!(parse(0, 2, r) && parse(2, 2, g) && parse(4, 2, b))) {
                return std::nullopt;
            }
            break;
        case 8:
            if (!(parse(0, 2, r) && parse(2, 2, g) && parse(4, 2, b) && parse(6, 2, a))) {
                return std::nullopt;
            }
            break;
        default:
            return std::nullopt;
        }

        return FromBytes(r, g, b, a);
    }

    /// Converts the colour into a hex colour code. If alwaysOutputAlpha is false, the alpha channel
    /// is only output if the colour is translucent.
    [[nodiscard]] std::string ToHex(bool alwaysOutputAlpha = false) const {
        const std::uint32_t argb = ToArgb();
        const auto a = static_cast<std::uint8_t>(argb >> 24);
        const auto r = static_cast<std::uint8_t>(argb >> 16);
        const auto g = static_cast<std::uint8_t>(argb >> 8);
        const auto b = static_cast<std::uint8_t>(argb);

        std::string result = "#";
        appendHexByte(result, r);
        appendHexByte(result, g);
        appendHexByte(result, b);
        if (alwaysOutputAlpha || a != 255) {
            appendHexByte(result, a);
        }
        return result;
    }

    /// Converts an HSV colour to a Colour4. All components should be in the range 0-1.
    [[nodiscard]] static Colour4 FromHsv(float hue, float saturation, float value,
                                         float alpha = 1.0F) {
        const int hi = static_cast<int>(hue * 6.0F);
        const float f = hue * 6.0F - static_cast<float>(hi);
        const float p = value * (1.0F - saturation);
        const float q = value * (1.0F - f * saturation);
        const float t = value * (1.0F - (1.0F - f) * saturation);

        switch (hi) {
        case 1:
            return {q, value, p, alpha};
        case 2:
            return {p, value, t, alpha};
        case 3:
            return {p, q, value, alpha};
        case 4:
            return {t, p, value, alpha};
        case 5:
            return {value, p, q, alpha};
        case 0:
        default:
            return {value, t, p, alpha};
        }
    }

    /// Converts the colour to the HSV colour space, as xyz of a vector with alpha in w. The angular
    /// hue is compressed to the 0-1 range in line with the other components.
    [[nodiscard]] glm::vec4 ToHsv() const {
        const float red = GetR();
        const float green = GetG();
        const float blue = GetB();

        const float maxC = std::max({red, green, blue});
        const float minC = std::min({red, green, blue});

        float hue = 0.0F;
        if (maxC == minC) {
            hue = 0.0F;
        } else if (maxC == red) {
            hue = std::fmod(6.0F + (green - blue) / (maxC - minC), 6.0F);
        } else if (maxC == green) {
            hue = (blue - red) / (maxC - minC) + 2.0F;
        } else {
            hue = (red - green) / (maxC - minC) + 4.0F;
        }

        const float saturation = maxC == 0.0F ? 0.0F : (maxC - minC) / maxC;
        hue = std::clamp(hue / 6.0F, 0.0F, 1.0F);

        return {hue == 1.0F ? 0.0F : hue, saturation, maxC, GetA()};
    }

    /// Converts an HSL colour to a Colour4. All components should be in the range 0-1.
    [[nodiscard]] static Colour4 FromHsl(float hue, float saturation, float lightness,
                                         float alpha = 1.0F) {
        const float c = (1.0F - std::abs(2.0F * lightness - 1.0F)) * saturation;
        const float h = hue * 6.0F;
        const float x = c * (1.0F - std::abs(std::fmod(h, 2.0F) - 1.0F));

        float r = 0.0F;
        float g = 0.0F;
        float b = 0.0F;

        if (0.0F <= h && h < 1.0F) {
            r = c;
            g = x;
        } else if (1.0F <= h && h < 2.0F) {
            r = x;
            g = c;
        } else if (2.0F <= h && h < 3.0F) {
            g = c;
            b = x;
        } else if (3.0F <= h && h < 4.0F) {
            g = x;
            b = c;
        } else if (4.0F <= h && h < 5.0F) {
            r = x;
            b = c;
        } else if (5.0F <= h && h <= 6.0F) {
            r = c;
            b = x;
        }

        const float m = lightness - c * 0.5F;
        return {r + m, g + m, b + m, alpha};
    }

    /// Converts the colour to the HSL colour space, as xyz of a vector with alpha in w. The angular
    /// hue is compressed to the 0-1 range in line with the other components.
    [[nodiscard]] glm::vec4 ToHsl() const {
        const float red = GetR();
        const float green = GetG();
        const float blue = GetB();

        const float maxC = std::max({red, green, blue});
        const float minC = std::min({red, green, blue});

        const float lightness = (maxC + minC) / 2.0F;
        if (lightness <= 0.0F) {
            return {0.0F, 0.0F, 0.0F, GetA()};
        }

        const float diff = maxC - minC;
        float saturation = diff;
        if (saturation <= 0.0F) {
            return {0.0F, 0.0F, lightness, GetA()};
        }

        saturation =
            lightness <= 0.5F ? saturation / (minC + maxC) : saturation / (2.0F - maxC - minC);

        float hue = 0.0F;
        if (maxC == red) {
            hue = (green - blue) / diff;
        } else if (maxC == green) {
            hue = (blue - red) / diff + 2.0F;
        } else if (maxC == blue) {
            hue = (red - green) / diff + 4.0F;
        }

        hue /= 6.0F;
        if (hue < 0.0F) {
            hue += 1.0F;
        }

        return {hue, saturation, lightness, GetA()};
    }

    [[nodiscard]] static constexpr Colour4 GetTransparent() { return FromBytes(255, 255, 255, 0); }
    [[nodiscard]] static constexpr Colour4 GetAliceBlue() { return FromBytes(240, 248, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetAntiqueWhite() {
        return FromBytes(250, 235, 215, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetAqua() { return FromBytes(0, 255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetAquamarine() { return FromBytes(127, 255, 212, 255); }
    [[nodiscard]] static constexpr Colour4 GetAzure() { return FromBytes(240, 255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetBeige() { return FromBytes(245, 245, 220, 255); }
    [[nodiscard]] static constexpr Colour4 GetBisque() { return FromBytes(255, 228, 196, 255); }
    [[nodiscard]] static constexpr Colour4 GetBlack() { return FromBytes(0, 0, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetBlanchedAlmond() {
        return FromBytes(255, 235, 205, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetBlue() { return FromBytes(0, 0, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetBlueViolet() { return FromBytes(138, 43, 226, 255); }
    [[nodiscard]] static constexpr Colour4 GetBrown() { return FromBytes(165, 42, 42, 255); }
    [[nodiscard]] static constexpr Colour4 GetBurlyWood() { return FromBytes(222, 184, 135, 255); }
    [[nodiscard]] static constexpr Colour4 GetCadetBlue() { return FromBytes(95, 158, 160, 255); }
    [[nodiscard]] static constexpr Colour4 GetChartreuse() { return FromBytes(127, 255, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetChocolate() { return FromBytes(210, 105, 30, 255); }
    [[nodiscard]] static constexpr Colour4 GetCoral() { return FromBytes(255, 127, 80, 255); }
    [[nodiscard]] static constexpr Colour4 GetCornflowerBlue() {
        return FromBytes(100, 149, 237, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetCornsilk() { return FromBytes(255, 248, 220, 255); }
    [[nodiscard]] static constexpr Colour4 GetCrimson() { return FromBytes(220, 20, 60, 255); }
    [[nodiscard]] static constexpr Colour4 GetCyan() { return FromBytes(0, 255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkBlue() { return FromBytes(0, 0, 139, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkCyan() { return FromBytes(0, 139, 139, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkGoldenrod() {
        return FromBytes(184, 134, 11, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetDarkGray() { return FromBytes(169, 169, 169, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkGreen() { return FromBytes(0, 100, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkKhaki() { return FromBytes(189, 183, 107, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkMagenta() { return FromBytes(139, 0, 139, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkOliveGreen() {
        return FromBytes(85, 107, 47, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetDarkOrange() { return FromBytes(255, 140, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkOrchid() { return FromBytes(153, 50, 204, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkRed() { return FromBytes(139, 0, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkSalmon() { return FromBytes(233, 150, 122, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkSeaGreen() {
        return FromBytes(143, 188, 139, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetDarkSlateBlue() {
        return FromBytes(72, 61, 139, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetDarkSlateGray() { return FromBytes(47, 79, 79, 255); }
    [[nodiscard]] static constexpr Colour4 GetDarkTurquoise() {
        return FromBytes(0, 206, 209, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetDarkViolet() { return FromBytes(148, 0, 211, 255); }
    [[nodiscard]] static constexpr Colour4 GetDeepPink() { return FromBytes(255, 20, 147, 255); }
    [[nodiscard]] static constexpr Colour4 GetDeepSkyBlue() { return FromBytes(0, 191, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetDimGray() { return FromBytes(105, 105, 105, 255); }
    [[nodiscard]] static constexpr Colour4 GetDodgerBlue() { return FromBytes(30, 144, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetFirebrick() { return FromBytes(178, 34, 34, 255); }
    [[nodiscard]] static constexpr Colour4 GetFloralWhite() {
        return FromBytes(255, 250, 240, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetForestGreen() { return FromBytes(34, 139, 34, 255); }
    [[nodiscard]] static constexpr Colour4 GetFuchsia() { return FromBytes(255, 0, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetGainsboro() { return FromBytes(220, 220, 220, 255); }
    [[nodiscard]] static constexpr Colour4 GetGhostWhite() { return FromBytes(248, 248, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetGold() { return FromBytes(255, 215, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetGoldenrod() { return FromBytes(218, 165, 32, 255); }
    [[nodiscard]] static constexpr Colour4 GetGray() { return FromBytes(128, 128, 128, 255); }
    [[nodiscard]] static constexpr Colour4 GetGreen() { return FromBytes(0, 128, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetGreenYellow() { return FromBytes(173, 255, 47, 255); }
    [[nodiscard]] static constexpr Colour4 GetHoneydew() { return FromBytes(240, 255, 240, 255); }
    [[nodiscard]] static constexpr Colour4 GetHotPink() { return FromBytes(255, 105, 180, 255); }
    [[nodiscard]] static constexpr Colour4 GetIndianRed() { return FromBytes(205, 92, 92, 255); }
    [[nodiscard]] static constexpr Colour4 GetIndigo() { return FromBytes(75, 0, 130, 255); }
    [[nodiscard]] static constexpr Colour4 GetIvory() { return FromBytes(255, 255, 240, 255); }
    [[nodiscard]] static constexpr Colour4 GetKhaki() { return FromBytes(240, 230, 140, 255); }
    [[nodiscard]] static constexpr Colour4 GetLavender() { return FromBytes(230, 230, 250, 255); }
    [[nodiscard]] static constexpr Colour4 GetLavenderBlush() {
        return FromBytes(255, 240, 245, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLawnGreen() { return FromBytes(124, 252, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetLemonChiffon() {
        return FromBytes(255, 250, 205, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightBlue() { return FromBytes(173, 216, 230, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightCoral() { return FromBytes(240, 128, 128, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightCyan() { return FromBytes(224, 255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightGoldenrodYellow() {
        return FromBytes(250, 250, 210, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightGreen() { return FromBytes(144, 238, 144, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightGray() { return FromBytes(211, 211, 211, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightPink() { return FromBytes(255, 182, 193, 255); }
    [[nodiscard]] static constexpr Colour4 GetLightSalmon() {
        return FromBytes(255, 160, 122, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightSeaGreen() {
        return FromBytes(32, 178, 170, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightSkyBlue() {
        return FromBytes(135, 206, 250, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightSlateGray() {
        return FromBytes(119, 136, 153, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightSteelBlue() {
        return FromBytes(176, 196, 222, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLightYellow() {
        return FromBytes(255, 255, 224, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetLime() { return FromBytes(0, 255, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetLimeGreen() { return FromBytes(50, 205, 50, 255); }
    [[nodiscard]] static constexpr Colour4 GetLinen() { return FromBytes(250, 240, 230, 255); }
    [[nodiscard]] static constexpr Colour4 GetMagenta() { return FromBytes(255, 0, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetMaroon() { return FromBytes(128, 0, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetMediumAquamarine() {
        return FromBytes(102, 205, 170, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumBlue() { return FromBytes(0, 0, 205, 255); }
    [[nodiscard]] static constexpr Colour4 GetMediumOrchid() {
        return FromBytes(186, 85, 211, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumPurple() {
        return FromBytes(147, 112, 219, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumSeaGreen() {
        return FromBytes(60, 179, 113, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumSlateBlue() {
        return FromBytes(123, 104, 238, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumSpringGreen() {
        return FromBytes(0, 250, 154, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumTurquoise() {
        return FromBytes(72, 209, 204, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMediumVioletRed() {
        return FromBytes(199, 21, 133, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetMidnightBlue() { return FromBytes(25, 25, 112, 255); }
    [[nodiscard]] static constexpr Colour4 GetMintCream() { return FromBytes(245, 255, 250, 255); }
    [[nodiscard]] static constexpr Colour4 GetMistyRose() { return FromBytes(255, 228, 225, 255); }
    [[nodiscard]] static constexpr Colour4 GetMoccasin() { return FromBytes(255, 228, 181, 255); }
    [[nodiscard]] static constexpr Colour4 GetNavajoWhite() {
        return FromBytes(255, 222, 173, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetNavy() { return FromBytes(0, 0, 128, 255); }
    [[nodiscard]] static constexpr Colour4 GetOldLace() { return FromBytes(253, 245, 230, 255); }
    [[nodiscard]] static constexpr Colour4 GetOlive() { return FromBytes(128, 128, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetOliveDrab() { return FromBytes(107, 142, 35, 255); }
    [[nodiscard]] static constexpr Colour4 GetOrange() { return FromBytes(255, 165, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetOrangeRed() { return FromBytes(255, 69, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetOrchid() { return FromBytes(218, 112, 214, 255); }
    [[nodiscard]] static constexpr Colour4 GetPaleGoldenrod() {
        return FromBytes(238, 232, 170, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetPaleGreen() { return FromBytes(152, 251, 152, 255); }
    [[nodiscard]] static constexpr Colour4 GetPaleTurquoise() {
        return FromBytes(175, 238, 238, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetPaleVioletRed() {
        return FromBytes(219, 112, 147, 255);
    }
    [[nodiscard]] static constexpr Colour4 GetPapayaWhip() { return FromBytes(255, 239, 213, 255); }
    [[nodiscard]] static constexpr Colour4 GetPeachPuff() { return FromBytes(255, 218, 185, 255); }
    [[nodiscard]] static constexpr Colour4 GetPeru() { return FromBytes(205, 133, 63, 255); }
    [[nodiscard]] static constexpr Colour4 GetPink() { return FromBytes(255, 192, 203, 255); }
    [[nodiscard]] static constexpr Colour4 GetPlum() { return FromBytes(221, 160, 221, 255); }
    [[nodiscard]] static constexpr Colour4 GetPowderBlue() { return FromBytes(176, 224, 230, 255); }
    [[nodiscard]] static constexpr Colour4 GetPurple() { return FromBytes(128, 0, 128, 255); }
    [[nodiscard]] static constexpr Colour4 GetRed() { return FromBytes(255, 0, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetRosyBrown() { return FromBytes(188, 143, 143, 255); }
    [[nodiscard]] static constexpr Colour4 GetRoyalBlue() { return FromBytes(65, 105, 225, 255); }
    [[nodiscard]] static constexpr Colour4 GetSaddleBrown() { return FromBytes(139, 69, 19, 255); }
    [[nodiscard]] static constexpr Colour4 GetSalmon() { return FromBytes(250, 128, 114, 255); }
    [[nodiscard]] static constexpr Colour4 GetSandyBrown() { return FromBytes(244, 164, 96, 255); }
    [[nodiscard]] static constexpr Colour4 GetSeaGreen() { return FromBytes(46, 139, 87, 255); }
    [[nodiscard]] static constexpr Colour4 GetSeaShell() { return FromBytes(255, 245, 238, 255); }
    [[nodiscard]] static constexpr Colour4 GetSienna() { return FromBytes(160, 82, 45, 255); }
    [[nodiscard]] static constexpr Colour4 GetSilver() { return FromBytes(192, 192, 192, 255); }
    [[nodiscard]] static constexpr Colour4 GetSkyBlue() { return FromBytes(135, 206, 235, 255); }
    [[nodiscard]] static constexpr Colour4 GetSlateBlue() { return FromBytes(106, 90, 205, 255); }
    [[nodiscard]] static constexpr Colour4 GetSlateGray() { return FromBytes(112, 128, 144, 255); }
    [[nodiscard]] static constexpr Colour4 GetSnow() { return FromBytes(255, 250, 250, 255); }
    [[nodiscard]] static constexpr Colour4 GetSpringGreen() { return FromBytes(0, 255, 127, 255); }
    [[nodiscard]] static constexpr Colour4 GetSteelBlue() { return FromBytes(70, 130, 180, 255); }
    [[nodiscard]] static constexpr Colour4 GetTan() { return FromBytes(210, 180, 140, 255); }
    [[nodiscard]] static constexpr Colour4 GetTeal() { return FromBytes(0, 128, 128, 255); }
    [[nodiscard]] static constexpr Colour4 GetThistle() { return FromBytes(216, 191, 216, 255); }
    [[nodiscard]] static constexpr Colour4 GetTomato() { return FromBytes(255, 99, 71, 255); }
    [[nodiscard]] static constexpr Colour4 GetTurquoise() { return FromBytes(64, 224, 208, 255); }
    [[nodiscard]] static constexpr Colour4 GetViolet() { return FromBytes(238, 130, 238, 255); }
    [[nodiscard]] static constexpr Colour4 GetWheat() { return FromBytes(245, 222, 179, 255); }
    [[nodiscard]] static constexpr Colour4 GetWhite() { return FromBytes(255, 255, 255, 255); }
    [[nodiscard]] static constexpr Colour4 GetWhiteSmoke() { return FromBytes(245, 245, 245, 255); }
    [[nodiscard]] static constexpr Colour4 GetYellow() { return FromBytes(255, 255, 0, 255); }
    [[nodiscard]] static constexpr Colour4 GetYellowGreen() { return FromBytes(154, 205, 50, 255); }

  private:
    static constexpr double kGamma = 2.4;

    static float toLinear(float color) {
        if (color == 1.0F) {
            return 1.0F;
        }
        return color <= 0.04045F ? color / 12.92F
                                 : std::pow((color + 0.055F) / 1.055F, static_cast<float>(kGamma));
    }

    static float toSrgb(float color) {
        if (color == 1.0F) {
            return 1.0F;
        }
        return color < 0.0031308F
                   ? 12.92F * color
                   : 1.055F * std::pow(color, 1.0F / static_cast<float>(kGamma)) - 0.055F;
    }

    /// 0-1 float -> 0-255 integer
    static std::uint32_t packByte(float component) {
        return static_cast<std::uint32_t>(std::clamp(component, 0.0F, 1.0F) * 255.0F);
    }

    static constexpr std::uint8_t expandNibble(std::uint8_t nibble) {
        return static_cast<std::uint8_t>(nibble * 17);
    }

    static bool parseHexByte(std::string_view digits, std::uint8_t& out) {
        const char* first = digits.data();
        const char* last = first + digits.size();
        const auto [ptr, ec] = std::from_chars(first, last, out, 16);
        return ec == std::errc() && ptr == last;
    }

    static void appendHexByte(std::string& out, std::uint8_t value) {
        constexpr std::string_view kDigits = "0123456789ABCDEF";
        out.push_back(kDigits[value >> 4]);
        out.push_back(kDigits[value & 0xF]);
    }

    glm::vec4 vector_{};
};

} // namespace panlid::graphics
