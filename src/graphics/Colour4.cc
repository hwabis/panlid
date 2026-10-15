#include "panlid/graphics/Colour4.hh"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace panlid::graphics {

namespace {

constexpr float kGamma = 2.4F;

float ToLinearComponent(float colour) {
    if (colour == 1) {
        return 1;
    }

    return colour <= 0.04045F ? colour / 12.92F : std::pow((colour + 0.055F) / 1.055F, kGamma);
}

float ToSrgbComponent(float colour) {
    if (colour == 1) {
        return 1;
    }

    return colour < 0.0031308F ? 12.92F * colour : 1.055F * std::pow(colour, 1.F / kGamma) - 0.055F;
}

std::optional<std::uint8_t> ParseHexByte(std::string_view digits) {
    std::uint8_t value = 0;
    const char* begin = std::to_address(digits.begin());
    const char* end = std::to_address(digits.end());
    const auto [ptr, error] = std::from_chars(begin, end, value, 16);

    if (error != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return value;
}

} // namespace

Colour4 Colour4::FromHex(std::string_view hex) {
    const auto colour = TryParseHex(hex);

    if (!colour) {
        throw std::invalid_argument(std::format("{} is not a valid colour hex string.", hex));
    }

    return *colour;
}

std::optional<Colour4> Colour4::TryParseHex(std::string_view hex) {
    if (hex.starts_with('#')) {
        hex.remove_prefix(1);
    }

    std::size_t digitsPerChannel = 0;

    switch (hex.size()) {
    case 3:
    case 4:
        digitsPerChannel = 1;
        break;

    case 6:
    case 8:
        digitsPerChannel = 2;
        break;

    default:
        return std::nullopt;
    }

    // Channels missing from the string (alpha in RGB and RRGGBB) stay at 255.
    std::array<std::uint8_t, 4> channels{255, 255, 255, 255};

    for (std::size_t i = 0; i < hex.size() / digitsPerChannel; ++i) {
        const auto value = ParseHexByte(hex.substr(i * digitsPerChannel, digitsPerChannel));

        if (!value) {
            return std::nullopt;
        }

        // A single digit stands for the doubled digit (#123 is #112233).
        channels[i] = digitsPerChannel == 1 ? static_cast<std::uint8_t>(*value * 17) : *value;
    }

    return FromBytes(channels[0], channels[1], channels[2], channels[3]);
}

std::string Colour4::ToHex(bool alwaysOutputAlpha) const {
    const std::uint32_t argb = ToArgb();
    const std::uint32_t a = argb >> 24;
    const std::uint32_t r = (argb >> 16) & 0xFF;
    const std::uint32_t g = (argb >> 8) & 0xFF;
    const std::uint32_t b = argb & 0xFF;

    if (!alwaysOutputAlpha && a == 255) {
        return std::format("#{:02X}{:02X}{:02X}", r, g, b);
    }

    return std::format("#{:02X}{:02X}{:02X}{:02X}", r, g, b, a);
}

Colour4 Colour4::FromHsv(float hue, float saturation, float value, float alpha) {
    const int hi = static_cast<int>(hue * 6);
    const float f = hue * 6.F - static_cast<float>(hi);
    const float p = value * (1 - saturation);
    const float q = value * (1 - f * saturation);
    const float t = value * (1 - (1 - f) * saturation);

    switch (hi) {
    case 1:
        return {.R = q, .G = value, .B = p, .A = alpha};

    case 2:
        return {.R = p, .G = value, .B = t, .A = alpha};

    case 3:
        return {.R = p, .G = q, .B = value, .A = alpha};

    case 4:
        return {.R = t, .G = p, .B = value, .A = alpha};

    case 5:
        return {.R = value, .G = p, .B = q, .A = alpha};

    default:
        return {.R = value, .G = t, .B = p, .A = alpha};
    }
}

glm::vec4 Colour4::ToHsv() const {
    const float max = std::max({R, G, B});
    const float min = std::min({R, G, B});

    float hue = 0.F;

    if (max != min) {
        if (max == R) {
            hue = std::fmod(6.F + (G - B) / (max - min), 6.F);
        } else if (max == G) {
            hue = (B - R) / (max - min) + 2;
        } else {
            hue = (R - G) / (max - min) + 4;
        }
    }

    const float saturation = max == 0 ? 0.F : (max - min) / max;
    hue = std::clamp(hue / 6.F, 0.F, 1.F);

    return {hue == 1.F ? 0.F : hue, saturation, max, A};
}

Colour4 Colour4::FromHsl(float hue, float saturation, float lightness, float alpha) {
    const float c = (1.F - std::abs(2.F * lightness - 1.F)) * saturation;
    const float h = hue * 6.F;
    const float x = c * (1.F - std::abs(std::fmod(h, 2.F) - 1.F));

    float r = 0.F;
    float g = 0.F;
    float b = 0.F;

    if (0.F <= h && h < 1.F) {
        r = c;
        g = x;
    } else if (1.F <= h && h < 2.F) {
        r = x;
        g = c;
    } else if (2.F <= h && h < 3.F) {
        g = c;
        b = x;
    } else if (3.F <= h && h < 4.F) {
        g = x;
        b = c;
    } else if (4.F <= h && h < 5.F) {
        r = x;
        b = c;
    } else if (5.F <= h && h <= 6.F) {
        r = c;
        b = x;
    }

    // Anything outside 0-6 leaves r, g and b at zero.
    const float m = lightness - c * 0.5F;
    return {.R = r + m, .G = g + m, .B = b + m, .A = alpha};
}

glm::vec4 Colour4::ToHsl() const {
    const float max = std::max({R, G, B});
    const float min = std::min({R, G, B});

    const float lightness = (max + min) / 2.F;

    if (lightness <= 0) {
        return {0.F, 0.F, 0.F, A};
    }

    const float diff = max - min;
    float saturation = diff;

    if (saturation <= 0) {
        return {0.F, 0.F, lightness, A};
    }

    saturation = lightness <= 0.5F ? saturation / (min + max) : saturation / (2.F - max - min);

    float hue = 0.F;

    if (max == R) {
        hue = (G - B) / diff;
    } else if (max == G) {
        hue = (B - R) / diff + 2.F;
    } else if (max == B) {
        hue = (R - G) / diff + 4.F;
    }

    hue /= 6.F;

    if (hue < 0.F) {
        hue += 1.F;
    }

    return {hue, saturation, lightness, A};
}

Colour4 Colour4::ToLinear() const {
    return {
        .R = ToLinearComponent(R), .G = ToLinearComponent(G), .B = ToLinearComponent(B), .A = A};
}

Colour4 Colour4::ToSrgb() const {
    return {.R = ToSrgbComponent(R), .G = ToSrgbComponent(G), .B = ToSrgbComponent(B), .A = A};
}

} // namespace panlid::graphics
